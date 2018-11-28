#!/usr/bin/python3

import os
import sys
import subprocess as sub
import json

def writeb(*args, **kwargs):
    """write binary data"""
    sys.stdout.buffer.write(*args, **kwargs)


class Event():
    """ event to register with perf probe """
    def __init__(self, executable, group, name, source_file_name, function, with_return):
        self.executable = executable
        self.name = name
        self.group = group
        self.line = None # add line support
        self.src = source_file_name
        self.function = function

        if with_return == None:
            self.with_return = True
        else:
            self.with_return = with_return

    def string_enter(self):
        rv = None
        if self.line:
            rv = "{0.name}={0.src}:{0.line}".format(self)
        else:
            rv = "{0.name}={0.function}@{0.src}".format(self)

        if self.group:
            rv = self.group + ":" + rv

        return rv

    def string_exit(self):
        if self.with_return:
            return self.string_enter() + "%return"
        else:
            return None

def parse_event_tree(event_list, event_tree, executable = None, group = None, source_file = None, with_return = None):
    if not isinstance(event_tree, list):
        raise ValueError("passed tree is not a list")

    for item in event_tree:
        executable = item.get("executable", executable)
        if item.get("functions", None):
            group = item.get("group", group)
            source_file = item["file"]
            parse_event_tree(event_list, item["functions"],
                             executable = executable,
                             group = group,
                             source_file = source_file,
                             with_return = with_return)
        elif item.get("files", None):
            group = item.get("group", group)
            parse_event_tree(event_list, item["files"],
                             executable = executable,
                             group = group,
                             source_file = source_file,
                             with_return = with_return)
        elif isinstance(item, list):
            parse_event_tree(event_list, item,
                             executable = executable,
                             group = group,
                             source_file = source_file,
                             with_return = with_return)
        else:
            parse_event_leaf(event_list, item,
                             executable = executable,
                             group = group,
                             source_file = source_file,
                             with_return = with_return)


def parse_event_leaf(event_list, leaf, executable = None, group = None, source_file = None, with_return = None):
    #print(json.dumps(leaf))
    function = leaf["function"]
    group = leaf.get("group", group)
    name = leaf.get("name", function)
    source = leaf.get("file", source_file)
    with_return = leaf.get("with_return", with_return)
    executable = leaf.get("executable", executable)

    event = Event(executable, group, name, source, function, with_return)
    event_list.append(event)

def parse_events(event_file):
    """ parse events from json file into events class """
    events = [] # to be returned
    event_tree = None # event_file read into memory

    # read into tree
    try:
        with open(event_file) as f:
            event_tree = json.load(f)
    except e as Exception:
        print("caught error while reading json file")
        raise e
    #print(json.dumps(event_tree, indent=4))

    # prase tree into event list
    if not isinstance(event_tree, list):
        print("json file does not contain array")
        sys.exit(1)

    ## TODO improve parsinga
    parse_event_tree(events, event_tree, "foo")
    return events


def setup_events(event_list):
    """ register all events in a list with perf by calling perf probe """
    to_add = []
    for event in event_list:
        for string in [ event.string_enter(), event.string_exit() ]:
            if string:
                to_add.append("--add")
                to_add.append(string)
                print(string)

    if to_add:
        with sub.Popen(["/usr/bin/perf", "probe", "-x", event.executable, *to_add  ], stdout=sub.PIPE) as proc:
            writeb(proc.stdout.read())


def main(event_file):
    events = parse_events(event_file)
    setup_events(events)
    return 0


if __name__ == "__main__":
    event_file = sys.argv[1]
    if event_file:
        sys.exit(main(event_file))
    else:
        print("please provide a file")

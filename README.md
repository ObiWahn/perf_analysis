# pbench - perf analysis tools
This project provides a small collection of
tools that aims to ease the use of perf.

## quick start
- setup events: `pbench_setup_events.py profile.json`
- convert to script output: `perf script -F comm,pid,tid,cpu,time,event,trace -i perf.data > perf.script`
- record events: `perf record -e '<groupname>:*' -aR`
- do final evaluation: `pbench_evalutate perf.script`

## detailed description

### pbench_setup
`pbench_setup` allows to setup events for perf using `groups`.
This `groups` can be used to filter events when recording `perf.data`.

Description of event.json consumed by `pbench_setup_events.py`
```json
[
    {
        "group": "<groupname>",
        "executable": "<path/to/executalbe>",
        "files": [
            {
                "file": "<source file>",
                "function": "<function name>"
            },
            {
                "file": "db_impl_compaction_flush.cc",
                "function": "BGWorkCompaction"
            }
        ]
    }
]
```

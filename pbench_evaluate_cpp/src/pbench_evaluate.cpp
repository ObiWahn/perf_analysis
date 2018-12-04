// ccopy
#include <string>
#include <iostream>
#include <fstream>
#include <charconv>
#include <map>
#include <unordered_map>

#include <tao/pegtl.hpp>


namespace pegtl = tao::pegtl;

//       Scheduler 10231/10238 [001] 2711293.466369:           group:function: (55d39097b05c)
//       Scheduler 10231/10238 [001] 2711293.466398:   group:function__return: (55d39097b05c <- 55d390e4cbbf)
//       ThreadName tid/pid    [cpu] ti.me:            group:function__return: (55d39097b05c <- 55d390e4cbbf)


struct event {
    event() = default;
    event(event&&) = default;
    event& operator=(event&&) = default;
    event(event const&) = delete;
    event& operator=(event const&) = delete;


    double time = 0;
    double duration = -1;
    std::uint32_t tid;
    //std::uint32_t pid;
    //std::string thread_name;
    std::string group_name;
    std::string function_name;
    //std::uint8_t  cpu;
    bool is_return = false;

    std::string id() const {
        return group_name + function_name + std::to_string(tid);
    }
};

std::ostream& operator<<(std::ostream& os, event const& e) {
    os  << e.function_name << "("<< e.tid << ")"<< " " << (e.is_return ? "exit " : "enter") << " " << std::fixed << e.duration;
    return os;
}

bool operator<(event const& left, event const& right) {
    std::cout << "comp - l: " << left << " r: " << right << std::endl;
    if(left.group_name < right.group_name) {
        return true;
    } else if(left.group_name > right.group_name) {
        return true;
    } else if(left.function_name < right.function_name) {
        return true;
    } else if(left.function_name > right.function_name) {
        return true;
    } else {
        return left.duration < right.duration;
    }
}

namespace pbench {
    using namespace pegtl;


    // basic rules
    using white = plus< space >;
    using syms = plus< alnum >;
    using digits = star< digit>;
    struct forward_slash : one<'/'>{};
    struct square_open : one<'['>{};
    struct square_close : one<']'>{};
    struct dot : one<'.'>{};
    struct colon : one<':'>{};
    struct brace_open : one<'('>{};
    struct brace_close : one<')'>{};
    struct floatnum : seq<digits, dot, digits>{};

    // used in actions
    struct tid_tok : syms {};
    struct pid_tok : syms {};
    struct cpu_tok : syms {};
    struct time_tok : floatnum {};
    struct group_tok : syms {};
    struct function_tok : syms {};
    struct ret_tok : string<'_','_','r','e','t','u','r','n'>{};

    // regular grammar
    struct grammar : seq<star<white>, syms, white
                        ,tid_tok, forward_slash, pid_tok, white
                        ,square_open, cpu_tok, square_close, white
                        ,time_tok, colon, white
                        ,group_tok, colon, function_tok, opt<ret_tok>, colon, white
                        > {};


    // default action
    template<typename rule>
    struct action : pegtl::nothing<rule>{};

    // actions for tokens
    template<>
    struct action<group_tok>{
        template <typename input>
        static void apply(input const& in, event& v){
            v.group_name = in.string();
        }
    };

    template<>
    struct action<function_tok>{
        template <typename input>
        static void apply(input const& in, event& v){
            v.function_name = in.string();
        }
    };

    template<>
    struct action<time_tok>{
        template <typename input>
        static void apply(input const& in, event& v){
            //auto rv = std::from_chars(in.begin(), in.end(), v.time, 10); //should work for dobule
            v.time = std::stod(in.string());
        }
    };

    template<>
    struct action<tid_tok>{
        template <typename input>
        static void apply(input const& in, event& v){
            auto rv = std::from_chars(in.begin(), in.end(), v.tid);
        }
    };

    template<>
    struct action<ret_tok>{
        static void apply0(event& v){
            v.is_return = true;
        }
    };
}

int main(int argc, char* argv[]){
    if ( argc <= 1) {
        return 1;
    }

    std::string filename(argv[1]);
    std::ifstream file(filename);

    std::unordered_map<std::string, event> new_events;
    std::vector<event> final_events;

    if (file.is_open()) {
        std::string line;
        while (getline(file, line)) {
            event new_event;
            pegtl::string_input in(line, filename);
            auto parse_result = pegtl::parse<pbench::grammar, pbench::action>(in, new_event);
            if(parse_result) {
                // std::cout << new_event << std::endl;
                auto id = new_event.id();
                auto found = new_events.find(id);

                if (found != new_events.end()){
                    if(new_event.is_return){
                        // update duration
                        auto& old_event = found->second;
                        old_event.duration = new_event.time - old_event.time;
                        // move to result
                        final_events.push_back(std::move(old_event));
                        new_events.erase(found);
                    } else {
                        // fail - double enter
                    }
                } else {
                    if(new_event.is_return) {
                        // fail - exit function before entering it
                    } else {
                        new_events.emplace(std::move(id),std::move(new_event));
                    }
                }
            } else {
                std::cerr << "failed to parse line: '" << line << "'" << std::endl;
            }
        }
        file.close();
    }

    for(auto const& event : final_events) {
        std::cout << event << std::endl;
        if(event.duration == 0 || event.function_name.empty()) {
            std::cout << "broken event";
            return 1;
        }
    }

    return 0;
}

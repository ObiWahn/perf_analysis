// ccopy
#include <string>
#include <iostream>
#include <fstream>
#include <charconv>
#include <unordered_map>
#include <iomanip>

#include <tao/pegtl.hpp>


namespace pegtl = tao::pegtl;

//       Scheduler 10231/10238 [001] 2711293.466369:           group:function: (55d39097b05c)
//       Scheduler 10231/10238 [001] 2711293.466398:   group:function__return: (55d39097b05c <- 55d390e4cbbf)
//       ThreadName tid/pid    [cpu] ti.me:            group:function__return: (55d39097b05c <- 55d390e4cbbf)


struct event {
    double time = 0;
    double duration = -1;
    std::uint32_t tid = 0;
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
    if(left.group_name < right.group_name) {
        return true;
    } else if(left.group_name > right.group_name) {
        return false;
    } else if(left.function_name < right.function_name) {
        return true;
    } else if(left.function_name > right.function_name) {
        return false;
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
            if (rv.ec == std::errc::invalid_argument) {
                std::cerr << "failed to convert tid" << std::endl;
            }
        }
    };

    template<>
    struct action<ret_tok>{
        static void apply0(event& v){
            v.is_return = true;
        }
    };
}

template <typename random_access_iterator>
void printStats(random_access_iterator begin, random_access_iterator end){
    if (begin == end){
        // empty range
        return;
    }
    std::cout << begin->group_name <<":"<< begin->function_name << std::endl;

    auto n = std::distance(begin, end);
    if (n < 2) {
        std::cout << "all: " << *begin << std::endl;
    }

    auto& min = *begin;
    auto& five = *(begin+(n/100)*5);
    auto& ten = *(begin+(n/100)*10);
    auto& med = *(begin+(n/2));
    auto& ninty = *(begin+(n/100)*90);
    auto& nintyfive = *(begin+(n/100)*95);
    auto& max = *(end-1);


    const double m = 1000000;
    std::cout << std::fixed << std::setprecision(2)
              << " MIN: " << min.duration * m
              << " - 5th: " << five.duration * m
              << " - 10th: " << ten.duration * m
              << " - MED: " << med.duration * m
              << " - 90th: " << ninty.duration * m
              << " - 95th: " << nintyfive.duration * m
              << " - MAX: " << max.duration * m
              << std::endl;
}

template <typename iter, typename predicate>
std::vector<iter> split_range(iter begin, iter end, predicate pred){
    std::vector<iter> rv;
    rv.push_back(begin);

    iter current = begin;
    while(current!=end){
        auto next = std::next(current);
        if (next != end) {
            if(pred(*current, *next)) {
                rv.push_back(next);
            }
        }
        current = next;
    }

    rv.push_back(end);
    return rv;
}

int main(int argc, char* argv[]){
    if ( argc <= 1) {
        return 1;
    }

    std::string filename(argv[1]);
    std::ifstream file(filename);

    std::unordered_map<std::string, event> new_events;
    std::vector<event> final_events;

    std::size_t linenumber = 0;
    if (file.is_open()) {
        std::string line;
        while (getline(file, line)) {
            ++linenumber;
            event new_event;
            pegtl::string_input in(line, filename);
            auto parse_result = pegtl::parse<pbench::grammar, pbench::action>(in, new_event);
            if(parse_result) {
                // std::cout << new_event << std::endl;
                std::string id = new_event.id();
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
                        std::cerr << "double enter of function in line: " << linenumber << std::endl
                                  << line << std::endl;
                    }
                } else {
                    if(new_event.is_return) {
                        std::cerr << "exiting function before enterning it in line: " << linenumber << std::endl
                                  << line << std::endl;
                    } else {
                        new_events.emplace(std::move(id),std::move(new_event));
                    }
                }
            } else {
                std::cerr << "failed to parse line(" << linenumber << "): '" << line << "'" << std::endl;
            }
        }
        file.close();
    }

    std::sort(final_events.begin(), final_events.end());
    auto split_points = split_range(final_events.begin(), final_events.end()
                                   ,[](auto l, auto r){ return l.group_name != r.group_name || l.function_name != r.function_name; }
                                   );
    for(auto point = split_points.begin(); std::next(point) != split_points.end(); ++point ) {
        printStats(*point, *std::next(point));
    }

    return 0;
}

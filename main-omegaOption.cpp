#include "src/detail/gfw_base.hpp"

#include <string>
#include <vector>

#include <iostream>

#include <re2/re2.h>

#include <sss/debug/value_msg.hpp>

struct test_case_t {
    const char * url;
    bool expect_use_proxy;
};

test_case_t test_case[] = {
    {"https://pica.zhimg.com/50/v2-6eb86e26636a39aca34a91dcd48c2b15_720w.jpg?source=1940ef5c", false},
    {"https://cloud.tencent.com/developer/article/1048338", false},
    {"https://ask.qcloudimg.com/http-save/yehe-1054460/8ar0q3332o.jpeg", false},
    {"https://www.google.com/", true},
    {"https://www.baidu.com/", false},

    {"t.co", true},
    {"g.co", true},
};

struct httpPolicy {
	std::vector<std::string> regs;
	std::vector<std::string> wildcards;

	int port;
	std::string host;
};

httpPolicy initRegexWrapperBlockedUrlList(const std::string& omegaOptionPath);

std::string omegaOptionsPath = "/Users/sarrow/Downloads/OmegaOptions.bak";

void testOmegaOptions() {
    using namespace varlisp::detail;
    auto gfwMgr = gfw_base::make_mgr(0, omegaOptionsPath);

    for(const auto curCase : test_case) {
        if (gfwMgr->need_proxy(curCase.url) != curCase.expect_use_proxy) {
            std::cout << curCase.url << ":" << curCase.expect_use_proxy << ":failed" << std::endl;
        }
    }
}

void testOmegaOptionsLoad() {
    auto policy = initRegexWrapperBlockedUrlList(omegaOptionsPath);
    std::cout << policy.regs.size() << std::endl;
    std::cout << policy.wildcards.size() << std::endl;
    for (size_t i = 0; i != 10 && i != policy.regs.size(); ++i) {
        std::cout << i << ":" << policy.regs[i] << std::endl;
    }
    for (size_t i = 0; i != 10 && i != policy.wildcards.size(); ++i) {
        std::cout << i << ":" << policy.wildcards[i] << std::endl;
    }
}

void testOmegaOptionsDetail() {
    using namespace varlisp::detail;
    auto policy = initRegexWrapperBlockedUrlList(omegaOptionsPath);

    RE2::Options regOptions;
    regOptions.set_max_mem(300 << 20); // NOTE for too large regex expression
    regOptions.set_one_line(true);
    regOptions.set_longest_match(true);

    auto tryRule = [&](const test_case_t& curCase, const std::vector<std::string>& patterns) {
        std::cout << SSS_VALUE_MSG(curCase.url) << "; " << SSS_VALUE_MSG(curCase.expect_use_proxy) << std::endl;
        if (!curCase.expect_use_proxy) {
            for (const auto& pat : patterns) {
                auto reg = RE2{pat, regOptions};
                if (gfw_need_proxy_with(reg, curCase.url) != curCase.expect_use_proxy) {
                    std::cout << "failed@" << SSS_VALUE_MSG(pat) << std::endl;
                    break;
                }
            }
        } else {
            bool match = false;
            for (const auto& pat : patterns) {
                auto reg = RE2{pat, regOptions};
                if (gfw_need_proxy_with(reg, curCase.url) == curCase.expect_use_proxy) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                std::cout << "all failed" << std::endl;
            }
        }
    };

    for(const auto curCase : test_case) {
        std::cout << "= policy.regs =" << std::endl;
        tryRule(curCase, policy.regs);
        std::cout << "- policy.wildcards -" << std::endl;
        tryRule(curCase, policy.wildcards);
    }
}

int main() {
    testOmegaOptionsLoad();
    testOmegaOptionsDetail();
    //testOmegaOptions();
}

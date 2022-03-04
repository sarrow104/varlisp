// src/detail/gfw_omegaoption.cpp
#include "gfw_omegaoption.hpp"
#include "consumer.hpp"

#include <sstream>
#include <cctype>
#include <string_view>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>

#include <re2/re2.h>

#include <ajson/ajson.hpp>
#include <ajson/ajson_helper.hpp>

#include <prettyprint.hpp>

#include <sss/path.hpp>
#include <sss/algorithm.hpp>
#include <sss/colorlog.hpp>

struct httpPolicy {
	std::vector<std::string> regs;
	std::vector<std::string> wildcards;

	int port;
	std::string host;
};

// "./OmegaOptions.bak"
httpPolicy initRegexWrapperBlockedUrlList(const std::string& omegaOptionPath);

namespace varlisp {
namespace detail {

// TODO 实现模板版本；
// 用typetraits 来实现不同类型数据的获取；
//
// TODO OmegaOption.bak的路径，得从varlisp的初始化脚本中获取；
// 也就是，得有一个全局的，varlisp解释器，然后，通过特殊的函数，可以获取；
// 另外，这个解释器对象，还得给一定的接口，用来获取特定类型的变量；
// 比如，int64,double，和 string等；
// 以方便其他语言使用；
std::string get_value_with_default(std::string name, std::string def);

gfw_omegaoption::gfw_omegaoption(std::string omegaOptionPath)
{
    COLOG_INFO("load OmegaOptions from:", omegaOptionPath);
    auto policy = initRegexWrapperBlockedUrlList(omegaOptionPath);

	RE2::Options regOptions;
	regOptions.set_max_mem(300 << 20); // NOTE for too large regex expression
	regOptions.set_one_line(true);
	regOptions.set_longest_match(true);

    this->urlWholeReg = std::make_shared<RE2>(boost::algorithm::join(policy.regs, "|")+ "|" +boost::algorithm::join(policy.wildcards, "|"), regOptions);
    this->host = policy.host;
    this->port = policy.port;
}

gfw_omegaoption::~gfw_omegaoption()
{
}

bool gfw_omegaoption::need_proxy(const std::string &url) const {
    return gfw_need_proxy_with(*this->urlWholeReg, url);
}

std::string gfw_omegaoption::get_host() const {
    return this->host;
}

int gfw_omegaoption::get_port() const {
    return this->port;
}

} // namespace detail
} // namespace varlisp

struct ConditionT {
	std::string conditionType;
	std::string pattern;
};

inline std::ostream& operator << (std::ostream& o, const ConditionT& c)
{
	o << "{ conditionType: " << c.conditionType << ", pattern: " << c.pattern << "}";
	return o;
}

AJSON(ConditionT, conditionType, pattern);

struct RuleT {
	ConditionT condition;
	std::string profileName; // "proxy"/"direct"
};

inline std::ostream& operator << (std::ostream& o, const RuleT& c)
{
	o << "{ condition: " << c.condition << ", profileName: " << c.profileName << "}";
	return o;
}

AJSON(RuleT, condition, profileName);

struct PlusAutoSwitchT {
	std::string profileType; // "SwitchProfile"

	std::vector<RuleT> rules;
	std::string name;
	std::string color;
	std::string defaultProfileName;
	std::string revision;
};

inline std::ostream& operator << (std::ostream& o, const PlusAutoSwitchT& c)
{
	o << "{"
		<< " profileType: " << c.profileType
		<< ", rules: " << c.rules
		<< ", mame: " << c.name
		<< ", color: " << c.color
		<< ", defaultProfileName: " << c.defaultProfileName
		<< ", revision: " << c.revision
		<< "}";
	return o;
}

AJSON(PlusAutoSwitchT, profileType, rules, name, color, defaultProfileName, revision);

struct ProxyT {
	int port;
	std::string scheme;
	std::string host;
};

inline std::ostream& operator << (std::ostream& o, const ProxyT& c)
{
	o << "{"
		<< " port: " << c.port
		<< ", scheme: " << c.scheme
		<< ", host: " << c.host
		<< "}";
	return o;
}

AJSON(ProxyT, port, scheme, host);

struct PlusProxyT {
	std::vector<ConditionT> bypassList;
	std::string profileType;
	std::string name;
	std::string color;
	ProxyT fallbackProxy;
	std::string revision;
};

inline std::ostream& operator << (std::ostream& o, const PlusProxyT& c)
{
	o << "{"
		<< " bypassList: " << c.bypassList
		<< ", profileType: " << c.profileType
		<< ", name: " << c.name
		<< ", color: " << c.color
		<< ", fallbackProxy: " << c.fallbackProxy
		<< "}";
	return o;
}

AJSON(PlusProxyT, bypassList, profileType, name, color, fallbackProxy, revision);

struct OmegaOptionT {
	std::map<std::string, std::string> ruleListOfAutoSwitch; // "+__ruleListOf_auto switch"
	 // - name:
	 // - defaultProfileName:
	 // - profileType:
	 // - color
	 // - format
	 // - matchProfileName:
	 // - ruleList: // -> the true url black-list and white-list
	 // - revision:
	 // - sourceUrl: https://raw.githubusercontent.com/gfwlist/gfwlist/master/gfwlist.txt
	 // - lastUpdate: 2021-07-20T15:08:02.290Z
	PlusAutoSwitchT plusAutoSwitch; // "+auto switch":{} | switch from OmegaOption
	PlusProxyT plusProxy; // "+proxy": {} | proxy info by user setting
	bool                     minusAddConditionsToBottom;  // "-addConditionsToBottom": false,
	bool                     minusConfirmDeletion;        // "-confirmDeletion": true,
	int                      minusDownloadInterval;       // "-downloadInterval": 1440,
	bool                     minusEnableQuickSwitch;      // "-enableQuickSwitch": false,
	bool                     minusMonitorWebRequests;     // "-monitorWebRequests": true,
	std::vector<std::string> minusQuickSwitchProfiles;    // "-quickSwitchProfiles": [],
	bool                     minusRefreshOnProfileChange; // "-refreshOnProfileChange": true,
	bool                     minusRevertProxyChanges;     // "-revertProxyChanges": true,
	bool                     minusShowExternalProfile;    // "-showExternalProfile": true,
	bool                     minusShowInspectMenu;        // "-showInspectMenu": true,
	std::string              minusStartupProfileName;     // "-startupProfileName": "",
	int                      schemaVersion;               // "schemaVersion": 2
};

inline std::ostream& operator << (std::ostream& o, const OmegaOptionT& c)
{
	o << "{"
		<< " ruleListOfAutoSwitch: " << c.ruleListOfAutoSwitch
		<< ", plusAutoSwitch: " << c.plusAutoSwitch
		<< ", plusProxy: " << c.plusProxy
		<< ", minusAddConditionsToBottom: " << c.minusAddConditionsToBottom
		<< ", minusConfirmDeletion: " << c.minusConfirmDeletion
		<< ", minusDownloadInterval: " << c.minusDownloadInterval
		<< ", minusEnableQuickSwitch: " << c.minusEnableQuickSwitch
		<< ", minusMonitorWebRequests: " << c.minusMonitorWebRequests
		<< ", minusQuickSwitchProfiles: " << c.minusQuickSwitchProfiles
		<< ", minusRefreshOnProfileChange: " << c.minusRefreshOnProfileChange
		<< ", minusRevertProxyChanges: " << c.minusRevertProxyChanges
		<< ", minusShowExternalProfile: " << c.minusShowExternalProfile
		<< ", minusShowInspectMenu: " << c.minusShowInspectMenu
		<< ", minusStartupProfileName: " << c.minusStartupProfileName
		<< ", schemaVersion: " << c.schemaVersion
		<< "}";
	return o;
}

AJSON(
		OmegaOptionT,
		ruleListOfAutoSwitch,
		plusAutoSwitch,
		plusProxy,

		minusAddConditionsToBottom,
		minusConfirmDeletion,
		minusDownloadInterval,
		minusEnableQuickSwitch,
		minusMonitorWebRequests,
		minusQuickSwitchProfiles,
		minusRefreshOnProfileChange,
		minusRevertProxyChanges,
		minusShowExternalProfile,
		minusShowInspectMenu,
		minusStartupProfileName,
		schemaVersion);

AJSON_FDNAME_MODIFIER(OmegaOptionT, ruleListOfAutoSwitch, "+__ruleListOf_auto switch");
AJSON_FDNAME_MODIFIER(OmegaOptionT, plusAutoSwitch, "+auto switch");
AJSON_FDNAME_MODIFIER(OmegaOptionT, plusProxy, "+proxy");

AJSON_FDNAME_MODIFIER(OmegaOptionT, minusAddConditionsToBottom, "-addConditionsToBottom");
AJSON_FDNAME_MODIFIER(OmegaOptionT, minusConfirmDeletion, "-confirmDeletion");
AJSON_FDNAME_MODIFIER(OmegaOptionT, minusDownloadInterval, "-downloadInterval");
AJSON_FDNAME_MODIFIER(OmegaOptionT, minusEnableQuickSwitch, "-enableQuickSwitch");
AJSON_FDNAME_MODIFIER(OmegaOptionT, minusMonitorWebRequests, "-monitorWebRequests");
AJSON_FDNAME_MODIFIER(OmegaOptionT, minusQuickSwitchProfiles, "-quickSwitchProfiles");
AJSON_FDNAME_MODIFIER(OmegaOptionT, minusRefreshOnProfileChange, "-refreshOnProfileChange");
AJSON_FDNAME_MODIFIER(OmegaOptionT, minusRevertProxyChanges, "-revertProxyChanges");
AJSON_FDNAME_MODIFIER(OmegaOptionT, minusShowExternalProfile, "-showExternalProfile");
AJSON_FDNAME_MODIFIER(OmegaOptionT, minusShowInspectMenu, "-showInspectMenu");
AJSON_FDNAME_MODIFIER(OmegaOptionT, minusStartupProfileName, "-startupProfileName");

// "./OmegaOptions.bak"
httpPolicy initRegexWrapperBlockedUrlList(const std::string& omegaOptionPath) {
	OmegaOptionT omegaOption;
	ajson::load_from_file(omegaOption, omegaOptionPath.c_str());

	// std::cout << omegaOption << std::endl;

	// std::cout << omegaOption.plusAutoSwitch.rules << std::endl;
	// std::cout << omegaOption.ruleListOfAutoSwitch["ruleList"].length() << std::endl;
	// ruleList.txt 格式说明（猜想）
	// 用到的格式：
	// |url
	// ||url
	// url
	// .partialUrl
	// @@||url
	// @@|url
	//
	// 另外，'*'可以出现在任何地方……
	//
	std::istringstream iss{omegaOption.ruleListOfAutoSwitch["ruleList"]};

    // url pattern normalize:
    // 1. remove `https?://` prefix
    // 2. replace '*' with `.*`
    // 2. replace '.' with `\\.`
	auto urlNormalize = [](std::string_view line) -> std::string {
		std::ostringstream oss;
		if (line.starts_with(std::string_view{"https://"})) { // string_view.starts_with() need C++20
			line.remove_prefix(8);
		} else if (line.starts_with("http://")) {
			line.remove_prefix(7);
		}
        if (!line.empty() && std::isalnum(line.front())) {
            oss << "\\b";
        }
		for (auto c: line) {
			switch (c) {
				case '*':
					oss << '.';
					oss << '*';
					break;

				case '.':
					oss << '\\';
					oss << '.';
					break;

				default:
					oss << c;
					break;
			}
		}
        if (!line.empty() && std::isalnum(line.back())) {
            oss << "\\b";
        }
		return oss.str();
	};

	std::string line;

	std::vector<std::string> regs;
	std::vector<std::string> wildcards;
	while (std::getline(iss, line)) {
		if (line.empty() || line.front() == '!' || line.front() == '[') {
			continue;
		}
		std::string_view sv{line};

		switch (sv.front()) {
			case '@': // treat as comment
				// nothing todo here
				break;

			case '|':
				while (sv.front() == '|') {
					sv = sv.substr(1);
				}

                // ||g.co
                // ||t.co
				line = urlNormalize(sv);
				wildcards.push_back(line);
				break;

			case '.':
				line = urlNormalize("*"+line);
				wildcards.push_back(line);
				break;

			case '/': // regex
				// NOTE 需要注意，不是所有预定义正则，是由http:// 开始！
				// 这种，通常不以^开头！
				// 可以考虑，添加一个 ".*://" 的前缀
				sv = sv.substr(1, sv.size() - 2);

				if (!sv.starts_with("^")) {
					line = "^\\w+://";
					line += sv;

					sv = line;
				}

				// NOTE 暂时不删除
				//
				// 如果regex 占用太多资源的时候，才考虑压缩的问题；
				//if (sv.starts_with("^")) {
				//	sv.remove_prefix(1);
				//}

				//if (sv.ends_with("$")) {
				//	sv.remove_suffix(1);
				//}
				regs.push_back(std::string{sv});
				break;

			default:
				break;
		}
	}

	// std::cout <<  << std::endl;
	for (const auto& setting: omegaOption.plusAutoSwitch.rules) {
		if (setting.profileName == "proxy") {
			// "conditionType": "HostWildcardCondition",
			// "pattern": "hitomi.la"
			if (setting.condition.conditionType == "HostWildcardCondition") {
				wildcards.push_back(urlNormalize(setting.condition.pattern));
			}
		}
	}

	// TODO extract proxy: url and prot
	//
	// 1. plusAutoSwitch 因为是自定义的，因此权限更高——也就是说，可能需要做成两段式。
	// 2. 对于http(s)开头的，fullUrl模式去掉协议头；——和正则表达式分开匹配；同时尾部也添加'.*'

	httpPolicy res;
	res.regs = regs;
	res.wildcards = wildcards;
	res.host = omegaOption.plusProxy.fallbackProxy.host;
	res.port = omegaOption.plusProxy.fallbackProxy.port;
	return res;
}

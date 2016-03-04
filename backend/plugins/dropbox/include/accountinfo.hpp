#ifndef ACCOUNTINFO_HPP
#define ACCOUNTINFO_HPP

#include <json/json.h>
#include <ostream>
#include <string>

namespace td
{

struct AccountInfo
{

	AccountInfo(const Json::Value &root) :
		uid(root["uid"].asInt64()),
		display_name(root["display_name"].asString()),
		name_details({
			root["name_details"]["familiar_name"].asString(),
			root["name_details"]["given_name"].asString(),
			root["name_details"]["surname"].asString()
		}),
		referral_link(root["referral_link"].asString()),
		country(root["country"].asString()),
		locale(root["locale"].asString()),
		email(root["email"].asString()),
		email_verified(root["email_verified"].asBool()),
		is_paired(root["is_paired"].asBool()),
		team({
			root["team"]["name"].asString(),
			root["team_id"]["name"].asString()
		}),
		quota_info({
			root["quota_info"]["shared"].asUInt64(),
			root["quota_info"]["quota"].asUInt64(),
			root["quota_info"]["normal"].asUInt64()
		})
	{}

	//The user's unique Dropbox ID.
	long long uid;

	//The user's display name.
	std::string display_name;

	struct
	{
		//The locale-dependent familiar name for the user.
		std::string familiar_name;

		//The user's given name.
		std::string given_name;

		//The user's surname.
		std::string surname;

	} name_details;

	//The user's <a href="https://www.dropbox.com/referrals" />referral</a> link.
	std::string referral_link;

	//The user's two-letter country code, if available.
	std::string country;

	//Locale preference set by the user (e.g. en-us).
	std::string locale;

	//The user's email address.
	std::string email;

	//If true, the user's email address has been verified to belong to that user.
	bool email_verified;

	//If true, there is a paired account associated with this user.
	bool is_paired;

	struct
	{
		//The name of the team the user belongs to.
		std::string name;

		//The ID of the team the user belongs to.
		std::string team_id;

	} team;

	struct
	{
		//The user's used quota in shared folders (bytes). If the user belongs to a team, this includes all usage contributed to the team's quota outside of the user's own used quota (bytes).
		unsigned long long shared;

		//The user's total quota allocation (bytes). If the user belongs to a team, the team's total quota allocation (bytes).
		unsigned long long quota;

		//The user's used quota outside of shared folders (bytes).
		unsigned long long normal;
	} quota_info;

}; //struct class AccountInfo

inline std::ostream &operator<<(std::ostream &out, const AccountInfo &info)
{
	out<<"{"<<std::endl;
	out<<"\t\"uid\": "<<info.uid<<","<<std::endl;
	out<<"\t\"display_name\": \""<<info.display_name<<"\","<<std::endl;
	out<<"\t\"name_details\": {"<<std::endl;
	out<<"\t\t\"familiar_name\": \""<<info.name_details.familiar_name<<"\","<<std::endl;
	out<<"\t\t\"given_name\": \""<<info.name_details.given_name<<"\","<<std::endl;
	out<<"\t\t\"surname\": \""<<info.name_details.surname<<"\""<<std::endl;
	out<<"\t\"},"<<std::endl;
	out<<"\t\"referral_link\": \""<<info.referral_link<<"\","<<std::endl;
	out<<"\t\"country\": \""<<info.country<<"\","<<std::endl;
	out<<"\t\"locale\": \""<<info.locale<<"\","<<std::endl;
	out<<"\t\"email\": \""<<info.email<<"\","<<std::endl;
	out<<"\t\"email_verified\": "<<info.email_verified<<","<<std::endl;
	out<<"\t\"is_paired\": "<<info.is_paired<<","<<std::endl;
	out<<"\t\"team\": {"<<std::endl;
	out<<"\t\t\"name\": \""<<info.team.name<<"\","<<std::endl;
	out<<"\t\t\"team_id\": \""<<info.team.team_id<<"\""<<std::endl;
	out<<"\t\"},"<<std::endl;
	out<<"\t\"quota_info\": {"<<std::endl;
	out<<"\t\t\"shared\": "<<info.quota_info.shared<<","<<std::endl;
	out<<"\t\t\"quota\": "<<info.quota_info.quota<<","<<std::endl;
	out<<"\t\t\"normal\": "<<info.quota_info.normal<<""<<std::endl;
	out<<"\t}"<<std::endl;
	out<<"}";
	return out;
}

} //end namespace td

#endif //ACCOUNTINFO_HPP

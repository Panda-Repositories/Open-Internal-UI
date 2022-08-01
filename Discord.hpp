////#include <iostream>
////#include <exception>
////#include <string>
////#include "curl.h"
////
////class DiscordCurlInitException : public std::exception { };
////
////static size_t WriteCallback(char* contents, size_t size, size_t nmemb, std::string* userp)
////{
////	userp->append((char*)contents, size * nmemb);
////	return size * nmemb;
////}
////
////
////class DiscordHttpResponse {
////public:
////	unsigned long status_code;
////	std::string response_body; // not some sort of fancy JSON object cuz i cba to do that rn
////};
////
////
////class DiscordClient {
////public:
////	const char* auth_token;
////
////	DiscordClient() { }
////
////	DiscordClient(const char* authToken)
////		: auth_token(authToken) { }
////
////	void join_guild(const char* inviteCode) {
////		make_request("POST", std::string().append("/invite/").append(inviteCode).c_str());
////	}
////private:
////	const char* apiBaseUrl = "https://discordapp.com/api/v6";
////
////
////	DiscordHttpResponse make_request(const char* httpMethod, const char* endpoint, const char* data = "{}") {
////
////		CURL* curl = curl_easy_init();
////		DiscordHttpResponse resp;
////
////		curl_easy_setopt(curl, CURLOPT_URL, std::string().append(apiBaseUrl).append(endpoint).c_str());
////
////		std::string responseBody;
////
////		struct curl_slist* headers = NULL;
////		headers = curl_slist_append(headers, "Content-Type: application/json");
////		headers = curl_slist_append(headers, std::string().append("Authorization: ").append(auth_token).c_str());
////		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
////		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, httpMethod);
////		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
////		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
////		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&resp.response_body);
////
////		CURLcode response = curl_easy_perform(curl);
////
////		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp.status_code);
////
////		curl_easy_cleanup(curl);
////
////		return resp;
////	}
////};
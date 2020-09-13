#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>
#include <vector>
#include <string>
#include <stdbool.h>

#include "json.hpp"

using namespace std;
using json = nlohmann::json;

int main() {
	printf("ut_json_array\n");
//	string json_c_str = "{\"list\": [0.234, 34.34234, 2434.0234]}";
//	string json_c_str = "{\"list\": [\"nguyen\", \"trong\", \"than\"]}";
//	string json_c_str = "{\"list\": [41234, 324, 453454]}";
	string json_c_str = "{\"list\": [{\"tag_1\": 41234}, {\"tag_2\": 324}, {\"tag_3\": 453454}]}";
	json json_obj = json::parse(json_c_str.c_str());
	json json_obj_list = json_obj["list"];
	if (json_obj_list.is_array()) {
		printf("array\n");
		cout << json_obj_list[0]["tag_1"] << endl;
		cout << json_obj_list[1]["tag_2"] << endl;
		cout << json_obj_list[2]["tag_3"] << endl;

		if (json_obj_list[0].find("tag_x") != json_obj_list[0].end()) {
			printf("tag_1: %d\n", json_obj_list[0]["tag_1"].get<int>());
		}

		printf("tag_2: %d\n", json_obj_list[1]["tag_2"].get<int>());
		printf("tag_3: %d\n", json_obj_list[2]["tag_3"].get<int>());
	}
	else {
		printf("NOT array\n");
	}

	string dump = json_obj.dump();
	printf("json_dump: %s\n", dump.c_str());
	return 0;
}

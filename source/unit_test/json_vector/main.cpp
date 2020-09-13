#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>
#include <vector>
#include <stdbool.h>

#include "json.hpp"

using namespace std;
using json = nlohmann::json;

typedef struct {
	int e_1;
	int e_2;
} c_struct_t;

string json_file_path = "json/json_test.js";
int file_json_obj;

vector<string> json_vector;
vector<c_struct_t> cobj_vector;

int json_to_cobj(string&, void*);
int cobj_to_json(string&, void*);

bool is_cobj_exist(vector<c_struct_t>& cobj_vector, void* cobj);
int init_cobj_vector(vector<c_struct_t>& cobj_vector);
int json_vector_add(vector<c_struct_t>& cobj_vector, void* cobj);
int json_vector_rm(vector<c_struct_t>& cobj_vector, void* cobj);

class json_vector {
public:
	struct c_obj_t {
		int e_1;
		int e_2;
	};

	json_vector () {}
	~json_vector () {}

private:
	vector<string> m_json_vector_pers;
	vector<struct c_obj_t> m_c_obj_vector_pers;

};

int main() {
	c_struct_t c_struct;
	c_struct_t c_struct_1;
	c_struct.e_1 = 11;
	c_struct.e_2 = 12;

	string temp;

	cobj_to_json(temp, &c_struct);
	cout << "[c_struct]" << temp << endl;

	json_to_cobj(temp, &c_struct_1);

	cobj_to_json(temp, &c_struct_1);
	cout << "[c_struct_1]" << temp << endl;

	init_cobj_vector(cobj_vector);
	json_vector_add(cobj_vector, &c_struct);

	c_struct.e_1 = 13;
	c_struct.e_2 = 14;
	json_vector_add(cobj_vector, &c_struct);

	c_struct.e_1 = 15;
	c_struct.e_2 = 16;
	json_vector_add(cobj_vector, &c_struct);

	json_vector_rm(cobj_vector, &c_struct);

	return 0;
}

int json_to_cobj(string& str, void* cobj) {
	json js = json::parse(str.c_str());
	c_struct_t* c_struct = (c_struct_t*)cobj;

	if (js.size() && cobj != NULL) {
		c_struct->e_1 = js["e_1"];
		c_struct->e_2 = js["e_2"];
	}
	else {
		cout << "[ERR]" << " invalib json string" << endl;
		return -1;
	}

	return 0;
}

int cobj_to_json(string& str, void* cobj) {
	if (cobj != NULL) {
		c_struct_t* c_struct = (c_struct_t*)cobj;

		json js = {
			{"e_1", c_struct->e_1},
			{"e_2", c_struct->e_2},
		};

		str.assign(js.dump());

		return 0;
	}

	return -1;
}

bool is_cobj_exist(vector<c_struct_t>& cobj_vector, void* cobj) {
	c_struct_t* c_struct = (c_struct_t*)cobj;
	for (int i = 0; i < cobj_vector.size(); i++) {
		if (cobj_vector[i].e_1 == c_struct->e_1 &&
				cobj_vector[i].e_2 == c_struct->e_2) {
			return true;
		}
	}
	return false;
}

int init_cobj_vector(vector<c_struct_t>& cobj_vector) {
	struct stat file_info;

	file_json_obj =  open(json_file_path.c_str(), O_RDONLY | O_CREAT, 0666);

	if (file_json_obj < 0) {
		cout << "[init_cobj_vector][ERR]" << " Can't Open file: " <<  json_file_path.c_str() << endl;
		return -2;
	}
	else {
		cout << "[init_cobj_vector][OK]" << " Opened file: " <<  json_file_path.c_str() << endl;
		fstat(file_json_obj, &file_info);

		uint32_t file_buffer_len = file_info.st_size + 1;
		char* file_buffer = (char*)malloc(file_buffer_len);

		if (file_buffer == NULL) {
			return -1;
		}

		memset(file_buffer, 0, file_buffer_len);

		pread(file_json_obj, file_buffer, file_info.st_size, 0);

		close(file_json_obj);

		cout << "file_info.st_size: " << file_info.st_size << endl;
		if (file_info.st_size > 0) {
			json j = json::parse(file_buffer);

			if (j.size()) {
				/* clear list */
				cobj_vector.clear();

				c_struct_t c_struct;
				string js_str;

				/* update new element */
				for (json::iterator it = j.begin(); it != j.end(); ++it) {
					js_str = *it;
					json_to_cobj(js_str, &c_struct);

					cobj_vector.push_back(c_struct);
					json_vector.push_back(js_str);
				}
			}
		}

		free(file_buffer);
	}

	return 0;
}

int json_vector_add(vector<c_struct_t>& cobj_vector, void* cobj) {
	c_struct_t c_struct;
	memcpy(&c_struct, cobj, sizeof(c_struct_t));
	if (is_cobj_exist(cobj_vector, &c_struct)) {
		cout << "[json_vector_add][WRN]" << " object is existed" << endl;
	}
	else {
		/* push COBJ to vector */
		cobj_vector.push_back(c_struct);

		string temp_str;
		cobj_to_json(temp_str, &c_struct);
		json_vector.push_back(temp_str);

		cout << "[json_vector_add][OK]" << temp_str << endl;

		/* save cobj to file */
		file_json_obj = open(json_file_path.c_str(), O_WRONLY | O_CREAT, 0666);

		if (file_json_obj < 0) {
			cout << "[json_vector_add][ERR]" << " Can't Open file: " <<  json_file_path.c_str() << endl;
			return -1;
		}
		else {
			cout << "[json_vector_add][OK]" << " Opened file: " <<  json_file_path.c_str() << endl;
			json j_vector(json_vector);
			string str_j_vector = j_vector.dump();
			pwrite(file_json_obj, str_j_vector.c_str(), str_j_vector.length() + 1, 0);
			close(file_json_obj);
		}

		return 0;
	}

	return -1;
}

int json_vector_rm(vector<c_struct_t>& cobj_vector, void* cobj) {
	c_struct_t c_struct;
	memcpy(&c_struct, cobj, sizeof(c_struct_t));

	for (int i = 0; i < cobj_vector.size(); i++) {
		if (cobj_vector[i].e_1 == c_struct.e_1 &&
				cobj_vector[i].e_2 == c_struct.e_2) {
			cout << "[json_vector_rm][OK]" << " index: " << i << endl;
			cobj_vector.erase(cobj_vector.begin() + i);
			json_vector.erase(json_vector.begin() + i);

			/* save cobj to file */
			file_json_obj = open(json_file_path.c_str(), O_WRONLY, 0666);

			if (file_json_obj < 0) {
				cout << "[json_vector_rm][ERR]" << " Can't Open file: " <<  json_file_path.c_str() << endl;
				return -1;
			}
			else {
				cout << "[json_vector_rm][OK]" << " Opened file: " <<  json_file_path.c_str() << endl;
				json j_vector(json_vector);
				string str_j_vector = j_vector.dump();

				cout << "[json_vector_rm][j_vector.dump()]" << str_j_vector << endl;

				/* clean file */
				ftruncate(file_json_obj, 0);

				/* write new data */
				pwrite(file_json_obj, str_j_vector.c_str(), str_j_vector.length() + 1, 0);

				close(file_json_obj);
			}

			return 0;
		}
	}

	return -1;
}

/* This Source Code Form is subject to the terms of the Mozilla
* Public License, v. 2.0. If a copy of the MPL was not distributed
* with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright 2015 Ilya Zemskov
*
* FILE: serialize.c
*
* DESCRIPTION: Manager passwords for Cybiko Xtreme (serialization)
*
* HISTORY:
*   July 22, 2015 Created by Ilya Zemskov aka pascal (pascal.ilya@gmail.com)
*/

#include <cywin.h>

#include "serialize.h"

static long _do_serialization_string(const char * str, char * output_str, long current_pos) {
	memcpy(output_str + current_pos, str, strlen(str));
	output_str[current_pos + strlen(str)] = '\n';	

	return strlen(str) + 1;
}

static long _do_deserialization_string(char * dest_str, const char * input_str, long current_pos) {
	char * find_str = strstr(input_str + current_pos, "\n");
	if (find_str != NULL && find_str - (input_str + current_pos) < MAX_NAME_PASS_LENGTH) {
		memcpy(dest_str, input_str + current_pos, find_str - (input_str + current_pos));
		dest_str[find_str - (input_str + current_pos)] = '\0';
		current_pos += find_str - (input_str + current_pos) + 1;

		return current_pos;
	}
	else
		return -1;
}

static void encrypt_decrypt(const char * key, const char * input, char * output, size_t length) {	
	size_t i;
	for (i = 0; i < length; i++) {
		output[i] = input[i] ^ key[i % strlen(key)];
	}
}

static int get_sum_code_string(const char * str) {
	int i;
	int sum = 0;
	for (i = 0; i < strlen(str); i++) {
		sum += (int) str[i];
	}

	return sum;
}

static int get_sum_code_from_string(const char * str) {
	// check int symbols
	int code_sum = 0;
	int code_sum_decimal = 1;
	int i;
	for (i = strlen(str) - 1; i >= 0; i--) {
		if (str[i] < '0' || str[i] > '9')
			return -1;

		code_sum += ((int) str[i] - '0') * code_sum_decimal;
		code_sum_decimal *= 10;
	}

	return code_sum;
}

void clear_database(struct database ** head) {
	struct database * iter = (*head);
	while (iter != NULL) {
		struct database * del_elem = iter;
		iter = iter->next;

		if (del_elem->menu_item != NULL) {
			cItem_dtor(del_elem->menu_item, FREE_MEMORY);
		}

		free(del_elem);
	}

	(*head) = NULL;
}

int serialization(struct database * head, const char * master_password) {
	struct database * iter;	
	long filesize = 0;
	int codes_sum = 0;

	// calc all data size and get sum code of all base
	iter = head;
	while (iter != NULL) {
		filesize += strlen(iter->name) + 1 + strlen(iter->pass) + 1;
		codes_sum += get_sum_code_string(iter->name) + get_sum_code_string(iter->pass);
		iter = iter->next;
	}

	if (filesize > 0) {
		char * output_str;
		long current_pos = 0;	
		struct FileOutput * ptr_file_output = (struct FileOutput *) malloc(sizeof(struct FileOutput));
		char * crypt_str;
		char buffer_sum[16];		

		FileOutput_ctor_Ex(ptr_file_output, DATABASE_FILE, TRUE);	

		// in start of result string write sum of codes other strings for check decrypt database
		sprintf(buffer_sum, "%d", codes_sum);
		filesize += strlen(buffer_sum) + 1;

		output_str = malloc(filesize + 1);
		crypt_str = malloc(filesize + 1);

		current_pos += _do_serialization_string(buffer_sum, output_str, current_pos);

		iter = head;
		while (iter != NULL) {
			current_pos += _do_serialization_string(iter->name, output_str, current_pos);
			current_pos += _do_serialization_string(iter->pass, output_str, current_pos);

			iter = iter->next;
		}
		output_str[filesize] = '\0';
		
		encrypt_decrypt(master_password, output_str, crypt_str, filesize);
		crypt_str[filesize] = '\0';

		FileOutput_write(ptr_file_output, crypt_str, filesize);	
		
		FileOutput_truncate(ptr_file_output, filesize);

		FileOutput_dtor(ptr_file_output, FREE_MEMORY);

		free(output_str);
		free(crypt_str);

		return 0;
	}

	return -1;
}

int deserialization(struct database ** head, const char * master_password) {
	struct database * tail = NULL;
	struct FileInput * ptr_file_input = (struct FileInput *) malloc(sizeof(struct FileInput));

	FileInput_ctor_Ex(ptr_file_input, DATABASE_FILE);
	if (FileInput_is_good(ptr_file_input)) {
		long current_pos = 0;
		long file_size = FileInput_get_size(ptr_file_input);

		if (file_size > 0) {
			long pos;
			char * crypt_str = malloc(file_size + 1);
			char * input_str = malloc(file_size + 1);
			char buffer_sum[64];
			int codes_sum = 0;
			int codes_sum_from_file = 0;

			FileInput_read(ptr_file_input, crypt_str, file_size);
			encrypt_decrypt(master_password, crypt_str, input_str, file_size);

			input_str[file_size] = '\0';

			// read sum
			pos = _do_deserialization_string(buffer_sum, input_str, current_pos);
			if (pos == -1) {
				free(input_str);
				free(crypt_str);

				FileInput_dtor(ptr_file_input, FREE_MEMORY);

				return -1;
			}
			current_pos = pos;

			codes_sum_from_file = get_sum_code_from_string(buffer_sum);		

			while (1) {
				// parse string				
				struct database * new_elem = (struct database *) malloc(sizeof(struct database));

				pos = _do_deserialization_string(new_elem->name, input_str, current_pos);
				if (pos == -1) {
					free(new_elem);
					break;
				}

				current_pos = pos;				

				pos = _do_deserialization_string(new_elem->pass, input_str, current_pos);
				if (pos == -1) {
					free(new_elem);
					break;
				}

				current_pos = pos;

				codes_sum += get_sum_code_string(new_elem->name) + get_sum_code_string(new_elem->pass); 

				new_elem->menu_item = NULL;
				new_elem->next = NULL;
				if ((*head) == NULL)
					(*head) = new_elem;

				if (tail != NULL)
					tail->next = new_elem;
				tail = new_elem;
			}

			free(input_str);
			free(crypt_str);

			if (codes_sum != codes_sum_from_file) {
				clear_database(head);

				FileInput_dtor(ptr_file_input, FREE_MEMORY);

				return -1;
			}
		}
		else {
			FileInput_dtor(ptr_file_input, FREE_MEMORY);

			return -1;
		}
	}
	else {
		FileInput_dtor(ptr_file_input, FREE_MEMORY);

		return -1;
	}
	
	FileInput_dtor(ptr_file_input, FREE_MEMORY);

	return 0;
}

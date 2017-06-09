#ifndef SERIALIZE_H__INCLUDE
#define SERIALIZE_H__INCLUDE

#define DATABASE_FILE "pass.txt"
#define MAX_NAME_PASS_LENGTH 100

struct database {
	char name[MAX_NAME_PASS_LENGTH];
	char pass[MAX_NAME_PASS_LENGTH];
	struct cItem * menu_item;
	struct database * next;
};

int serialization(struct database * head, const char * master_password);
int deserialization(struct database ** head, const char * master_password);

void clear_database(struct database ** head);

#endif
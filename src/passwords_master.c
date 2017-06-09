/* This Source Code Form is subject to the terms of the Mozilla
* Public License, v. 2.0. If a copy of the MPL was not distributed
* with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright 2015 Ilya Zemskov
*
* FILE: passwords_master.c
*
* DESCRIPTION: Manager passwords for Cybiko Xtreme
*
* HISTORY:
*   July 17, 2015 Created by Ilya Zemskov aka pascal (pascal.ilya@gmail.com)
*/

#include <cywin.h>

#include "serialize.h"

/*
* FUNCTION: check_database
*
* DESCRIPTION: check password database on filesystem
*
* RETURNS: 0 if database exists, and -1 otherwise
*/
int check_database() {
	struct FileInput * ptr_file_input = (struct FileInput *) malloc(sizeof(struct FileInput));

	FileInput_ctor_Ex(ptr_file_input, DATABASE_FILE);
	if (FileInput_is_good(ptr_file_input)) {
		FileInput_dtor(ptr_file_input, FREE_MEMORY);

		return 0;
	}

	FileInput_dtor(ptr_file_input, FREE_MEMORY);

	return -1;
}

/*
* FUNCTION: get_master_password
*
* DESCRIPTION: show dialog for get password from user
*
* PARAMETERS:
*   ptr_form - parrent form
*   master_password
*/
void get_master_password(struct cFrameForm * ptr_form, char * master_password) {
	// Show dialog enter master password
	struct cDialog * ptr_pass_dialog;
	ptr_pass_dialog = (struct cDialog *) malloc(sizeof(struct cDialog));
	cDialog_ctor(ptr_pass_dialog, 0, "Enter master password", mbEdit | mbOk | mbCancel | mbs1, 15, ptr_form->CurrApplication);

	cDialog_SetEditText(ptr_pass_dialog, "");

	do {
		// If pressed cancel - close application
		if (cDialog_ShowModal(ptr_pass_dialog) == mbOk)
			cDialog_GetEditText(ptr_pass_dialog, master_password);
		else {
			ptr_form->ModalResult = mrQuit;
			break;
		}
	}
	while (strlen(master_password) == 0);

	// Releases the dialog
	cDialog_dtor(ptr_pass_dialog, FREE_MEMORY);
}

/*
* FUNCTION: check_password
*
* DESCRIPTION: check password
*
* PARAMETERS:
*   head - ptr to start list of passwords
*   master_password
*
* RETURNS: 0 if password true, and -1 otherwise
*/
int check_password(struct database ** head, const char * master_password) {
	// password check through deserialization database
	if (deserialization(head, master_password) == 0)
		return 0;

	return -1;
}

/*
* FUNCTION: render_database
*
* DESCRIPTION: show dialog for get password from user
*
* PARAMETERS:
*   ptr_form - parrent form
*   head - ptr to start list of passwords
*   ptr_menu_list - main list of passwords
*/
void render_database(struct cFrameForm * ptr_form, struct database * head, struct cList ** ptr_menu_list, struct cItem ** menu_item_add) {
	struct database * iter;		
	
	// Initializes the list object and fills it with items
	(*ptr_menu_list) = (struct cList*) malloc(sizeof(struct cList));
	cList_ctor((*ptr_menu_list), SCREEN_WIDTH - 4);

	// menu item - add password
	(*menu_item_add) = (struct cItem *) malloc(sizeof(struct cItem));
	cItem_ctor((*menu_item_add), SCREEN_WIDTH - 4, "New password", FALSE, NULL, NULL);
	cList_AddItem((*ptr_menu_list), (*menu_item_add));

	iter = head;
	while (iter != NULL) {
		iter->menu_item = (struct cItem *) malloc(sizeof(struct cItem));
		cItem_ctor(iter->menu_item, SCREEN_WIDTH - 4, iter->name, FALSE, NULL, NULL);
		cList_AddItem((*ptr_menu_list), iter->menu_item);
	
		iter = iter->next;
	}
	
	cFrameForm_AddObj(ptr_form, (*ptr_menu_list), 0, 0);
}

void add_new_password_dialog(struct cFrameForm * ptr_form, struct cList * ptr_menu_list, struct database ** head) {
	struct cDialog * ptr_add_pass_dialog;

	// create new elem
	struct database * new_elem = (struct database *) malloc(sizeof(struct database));
	new_elem->menu_item = NULL;
	new_elem->next = NULL;

	ptr_add_pass_dialog = (struct cDialog *) malloc(sizeof(struct cDialog));
	cDialog_ctor(ptr_add_pass_dialog, 0, "Set password name", mbEdit | mbOk | mbCancel | mbs1, 15, ptr_form->CurrApplication);	

	cDialog_SetEditText(ptr_add_pass_dialog, "");
	do {
		if (cDialog_ShowModal(ptr_add_pass_dialog) == mbOk) {	
			cDialog_GetEditText(ptr_add_pass_dialog, new_elem->name);
		}
		else {
			free(new_elem);
			new_elem = NULL;
			break;
		}
	}
	while (strlen(new_elem->name) == 0);

	// Releases the dialog
	cDialog_dtor(ptr_add_pass_dialog, FREE_MEMORY);

	// get password
	if (new_elem != NULL) {		
		ptr_add_pass_dialog = (struct cDialog *) malloc(sizeof(struct cDialog));
		cDialog_ctor(ptr_add_pass_dialog, 0, "Set password", mbEdit | mbOk | mbCancel | mbs1, 15, ptr_form->CurrApplication);
		
		cDialog_SetEditText(ptr_add_pass_dialog, "");		
		do {
			if (cDialog_ShowModal(ptr_add_pass_dialog) == mbOk) {
				cDialog_GetEditText(ptr_add_pass_dialog, new_elem->pass);
			}
			else {
				free(new_elem);
				new_elem = NULL;
				break;
			}
		}
		while (strlen(new_elem->pass) == 0);

		cDialog_dtor(ptr_add_pass_dialog, FREE_MEMORY);
	}

	// add password to list
	if (new_elem != NULL) {
		if ((*head) == NULL)
			(*head) = new_elem;
		else {
			// get tail
			struct database * tail = (*head);
			while (tail->next != NULL) {
				tail = tail->next;
			}

			tail->next = new_elem;			
		}

		// add to list
		new_elem->menu_item = (struct cItem *) malloc(sizeof(struct cItem));
		cItem_ctor(new_elem->menu_item, SCREEN_WIDTH - 4, new_elem->name, FALSE, NULL, NULL);
		cList_AddItem(ptr_menu_list, new_elem->menu_item);
	}
}

/*
* FUNCTION: render_password_dialog
*
* DESCRIPTION: render password dialog
*
* PARAMETERS:
*   ptr_form - parrent form
*   head - ptr to start list of passwords
*   index - index in list
*/
void render_password_dialog(struct cFrameForm * ptr_form, struct database * head, int index) {
	int i = 0;
	struct database * iter;	

	iter = head;
	while (i != index && iter != NULL) {
		iter = iter->next;
		i++;
	}

	if (iter != NULL) {
		struct cDialog * ptr_dialog = (struct cDialog *) malloc(sizeof(struct cDialog));
		cDialog_ctor(ptr_dialog, iter->name, iter->pass, mbOk | mbs1, 0, ptr_form->CurrApplication);
		cDialog_ShowModal(ptr_dialog);
		cDialog_dtor(ptr_dialog, FREE_MEMORY);
	}
}

/*
* FUNCTION: delete_password
*
* DESCRIPTION: delete_password
*
* PARAMETERS:
*   ptr_form - parrent form
*   head - ptr to start list of passwords
*   index - index in list
*/
void delete_password(struct cFrameForm * ptr_form, struct database ** head, int index, struct cList * ptr_menu_list) {
	int i = 0;
	struct database * iter;	
	struct database * prev_iter = NULL;

	iter = (*head);
	while (i != index && iter != NULL) {
		prev_iter = iter;
		iter = iter->next;
		i++;
	}

	if (iter != NULL) {
		struct cDialog * ptr_dialog = (struct cDialog *) malloc(sizeof(struct cDialog));
		cDialog_ctor(ptr_dialog, "Confirm delete", "Are you sure to delete the password?", mbYes | mbNo | mbs1, 0, ptr_form->CurrApplication);

		if (cDialog_ShowModal(ptr_dialog) == mrYes) {
			// remove elem from base
			if (iter == (*head))
				(*head) = (*head)->next;
			else
				prev_iter->next = iter->next;

			cList_RemItem(ptr_menu_list, iter->menu_item);
			cItem_dtor(iter->menu_item, FREE_MEMORY);

			free(iter);
		}

		cDialog_dtor(ptr_dialog, FREE_MEMORY);
	}
}

/*
* FUNCTION: init_main_form
*
* DESCRIPTION: initialize main form
*
* PARAMETERS:
*   module - module form
*   ptr_form - parent form
*
* RETURNS: 0 if success, and -1 otherwise
*/
int init_main_form(struct module_t & module, struct cFrameForm * ptr_form) {
	if (ptr_form == NULL)
		return -1;

	if (cFrameForm_ctor(ptr_form, "Passwords Master", module.m_process) == NULL)
		return -1;

	ptr_form->HelpContext = 0;	

	return 0;
}

/*
* FUNCTION: main_loop
*
* DESCRIPTION: proccess messages
*
* PARAMETERS:
*   module - module form
*   ptr_form - parent form
*
* RETURNS: void
*/
void main_loop(struct module_t & module, struct cFrameForm * ptr_form) {
	char master_password[100];

	struct KeyParam * ptr_key_param;
	struct cDialog * ptr_quit_dialog;

	struct cList * ptr_menu_list= NULL;
	struct cItem * menu_item_add = NULL;

	struct database * head = NULL;
	
	if (check_database() == 0) {
		// if database is exists get master password from dialog and try decrypt database
		do {
			get_master_password(ptr_form, master_password);
		}
		while (ptr_form->ModalResult != mrQuit && check_password(&head, master_password) != 0);

		// if not click in cancel button in password dialog and decrypt database is ok - render database to list
		if (ptr_form->ModalResult != mrQuit) {
			render_database(ptr_form, head, &ptr_menu_list, &menu_item_add);
		}
	}
	else {
		// if database not exist only safe entered password
		get_master_password(ptr_form, master_password);

		// add to list item "Add password"
		if (ptr_form->ModalResult != mrQuit) {
			render_database(ptr_form, head, &ptr_menu_list, &menu_item_add);
		}
	}

	if (ptr_form->ModalResult != mrQuit) {
		cFrameForm_Show(ptr_form);
		ptr_form->ModalResult = mrNone;
	}

	while (ptr_form->ModalResult != mrQuit) {
		struct Message * ptr_message = cWinApp_get_message(ptr_form->CurrApplication, 0, 1, MSG_USER);

		switch(ptr_message->msgid) {
			case MSG_SHUTUP:            // Processes the system exit signal.
			case MSG_QUIT:
				ptr_form->ModalResult = mrQuit;
			break;

			case MSG_KEYDOWN: // Processes keyboard input
				ptr_key_param = Message_get_key_param(ptr_message);
				switch(ptr_key_param->scancode) {
					case KEY_ESC:
						// Creates the dialog box with <Quit> and <Cancel> buttons
						ptr_quit_dialog = (struct cDialog *) malloc(sizeof(struct cDialog));

						cDialog_ctor(ptr_quit_dialog, 0, str_Really_exit, mbQuit | mbCancel | mbs1, 0, ptr_form->CurrApplication);

						if (cDialog_ShowModal(ptr_quit_dialog) == mrQuit)
							ptr_form->ModalResult = mrQuit;

						cDialog_dtor(ptr_quit_dialog, FREE_MEMORY);
					break;

					case KEY_ENTER:
						if (cList_Sel(ptr_menu_list) != 0)
							render_password_dialog(ptr_form, head, cList_Sel(ptr_menu_list) - 1);
						else
							add_new_password_dialog(ptr_form, ptr_menu_list, &head);
					break;

					case KEY_DEL:
						if (cList_Sel(ptr_menu_list) != 0)
							delete_password(ptr_form, &head, cList_Sel(ptr_menu_list) - 1, ptr_menu_list);
					break;

					default:
						cFrameForm_proc(ptr_form, ptr_message);
				}

			break;

			default:
				cWinApp_defproc(module.m_process, ptr_message);
		}

		Message_delete(ptr_message);
	}

	if (head != NULL) {
		serialization(head, master_password);
		clear_database(&head);
	}

	if (ptr_menu_list != NULL) {
		cItem_dtor(menu_item_add, FREE_MEMORY);
		cFrameForm_RemObj(ptr_form, ptr_menu_list);
		cList_dtor(ptr_menu_list, FREE_MEMORY);
	}
}

/*
* FUNCTION: main
*
* DESCRIPTION: program entry point
*
* PARAMETERS:
*   argc - number of arguments
*   argv - array of 'argc' arguments passed to the application
*   start - TRUE if the application is being initialized, FALSE otherwise
*
* RETURNS: 0L
*/
long main (int argc, char * argv[], bool start) {
	struct module_t main_module;               // Application's main module
	struct cFrameForm * ptr_main_form;         // Application's main form

	// Init the application
	init_module(&main_module);

	ptr_main_form = malloc(sizeof(struct cFrameForm));

	// init and show main form
	if (init_main_form(main_module, ptr_main_form) == 0) {
		// proccess messages
		main_loop(main_module, ptr_main_form);

		// free main form memory
		cFrameForm_dtor(ptr_main_form, FREE_MEMORY);
	}
	else
		free(ptr_main_form);		

	return 0L;
}
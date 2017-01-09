/*******************************************************************************
**
**  Name        BTE_HT_API.h
**  $Header: $
**
**  Function    BTE Insight Handset Tool API functions declarations
**
**  Date        Modification
**  ------------------------
**  06/09/2003		Create
**	06/10/03		Some datatypes and variables names have been changed;
**					HT_ChangeDisplayPriority function has been added
**	06/11/03		Function names changed - now it uses mixedcase in its names;
**					added two functions: HT_GetMenuSelectedItem and
**					HT_GetEditBoxCurrentText; Added one additional parameter
**					to the HT_ShowMenu function.
**	06/12/03		HT_ShowEditBox, HT_ShowMenu functions was deleted;
**					HT_GetSelectedMenuItem, HT_CreateMenu, HT_CreateEditbox
**					functions was changed.
**	06/12/03		Combine and simplify the setting of soft key behavior.
**	06/24/03		Deleted message box flags
**
**  Copyright (c) 1999, 2001, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*******************************************************************************/
#ifndef __BTE_HT_API_H__
#define __BTE_HT_API_H__

#ifdef BTE_HT_API_EXPORTS
#define BTE_HT_API __declspec(dllexport)
#else
#define BTE_HT_API
#endif

/*#include "data_types.h"*/

#ifdef __cplusplus
extern "C" {
#endif

/* Edit box definitions */
#define HT_EB_NUMBERS			0x0001
#define HT_EB_LETTERS			0x0002
#define HT_EB_NUMBERS_FLOAT		0x0004

/* Enum handset keys */
enum
{
	HT_KEY_0,
	HT_KEY_1,
	HT_KEY_2,
	HT_KEY_3,
	HT_KEY_4,
	HT_KEY_5,
	HT_KEY_6,
	HT_KEY_7,
	HT_KEY_8,
	HT_KEY_9,
	HT_KEY_ASTERISK,
	HT_KEY_POUND,
	HT_KEY_SELECT,
	HT_KEY_CLEAR,
	HT_KEY_DOWN,
	HT_KEY_UP,
	HT_KEY_SOFT1,
	HT_KEY_SOFT2,
	HT_KEY_SEND,
	HT_KEY_END,
	HT_NO_KEY,
	HT_KEY_LEFT,
	HT_KEY_RIGHT,

    HT_KEY_NUM_KEYS
}; typedef UINT8 HT_Keys;

/* Menu item structure */
typedef struct
{
	UINT8	*p_item_name;	/* menu item display name */
	UINT32	item_data;		/* menu item data, used by user */
	UINT32	item_index;		/* menu item original index */
} tHT_MENU_ITEM;


/*******************************************************************************
**
**	Function	tHT_KEY_PRESSED_CBACK
**
**	Parameters	handle - object menu, edit box or message box handle
**				key    - pressed key code
**
**	Returns		none
**
**	Remarks		User defined callback function.
**
*******************************************************************************/
typedef void (tHT_KEY_PRESSED_CBACK)(UINT32 handle, UINT32 key);

/*******************************************************************************
**
**	Function	HT_SetCallBack
**
**	Parameters	menu - Pointer to function called for the menus.
**				edit - Pointer to function called for the edit boxs.
**				tout - Pointer to function called for the timeouts.
**				mbox - Pointer to function called for the message boxs.
**
**	Returns		none
**
**	Remarks		Sets the Handsets callback functions.
**
*******************************************************************************/
BTE_HT_API extern void HT_SetCallBack(tHT_KEY_PRESSED_CBACK *menu,
    tHT_KEY_PRESSED_CBACK *edit, tHT_KEY_PRESSED_CBACK *tout,
    tHT_KEY_PRESSED_CBACK *mbox);

/*******************************************************************************
**
**	Function	HT_SetSoftKeyMessages
**
**	Parameters	handle - previously created display object
**				p_message_key_soft_1 - first soft key display text
**				p_message_key_soft_2 - second soft key display text
**
**	Returns		TRUE if succeeded, FALSE if incorrect strings are passed
**				as parameters
**
**	Remarks		This function should be called after any display object
**				is created to set the soft key behavior.
**
*******************************************************************************/
BTE_HT_API extern BOOLEAN HT_SetSoftKeyMessages(UINT32 handle,
	UINT8 *p_message_key_soft_1, UINT8 *p_message_key_soft_2);

/*******************************************************************************
**
**	Function	HT_CloseObject
**
**	Parameters	handle - valid open object
**
**	Returns		TRUE if object is deleted, FALSE if invalid object is given
**
**	Remarks		This function deletes previously created menu, edit box
**				or message box objects.
**
*******************************************************************************/
BTE_HT_API extern BOOLEAN HT_CloseObject(UINT32 handle);

/*******************************************************************************
**
**	Function	HT_ChangeDisplayPriority
**
**	Parameters	handle - existing object to be changed, menu, edit box
**					or message box
**				new_priority - the new priority of object in stack
**
**	Returns		TRUE if succeeded, FALSE if incorrect parameters are passed
**
**	Remarks		This function changes object priority in object stack.
**				Calling this function with parameters (handle, 0) forces
**				the object associated with handle to be shown.
**				The function returns immediately.
**
*******************************************************************************/
BTE_HT_API extern BOOLEAN HT_ChangeDisplayPriority(UINT32 handle, UINT8 new_priority);

/*******************************************************************************
**
**	Function	HT_CreateMenu
**
**	Parameters	p_menu_items - array of tHT_MENU_ITEM
**				menu_item_count - count of tHT_MENU_ITEM in previous
**					parameter
**				p_prompt_text - title of menu, for example
**					"Select phone:"
**
**	Returns		handle unique to menu object, or NULL if incorrect
**				parameters are used; function returns immediately.
**
**	Remarks		This function initializes a menu object.
**
*******************************************************************************/
BTE_HT_API extern UINT32 HT_CreateMenu(tHT_MENU_ITEM *p_menu_items,
	UINT32 menu_item_count, UINT8 *p_prompt_text);

/*******************************************************************************
**
**	Function	HT_SetMenuSelectedItem
**
**	Parameters	handle - previously created menu
**				item_index - index of an item to select
**
**	Returns		TRUE if succeeded, FALSE if incorrect strings are passed as
**				parameters
**
**	Remarks		This function selects an item.
**
*******************************************************************************/
BTE_HT_API extern BOOLEAN HT_SetMenuSelectedItem(UINT32 handle, UINT32 item_index);

/*******************************************************************************
**
**	Function	HT_GetMenuSelectedItem
**
**	Parameters	handle - previously created menu
**
**	Returns		tHT_MENU_ITEM of currently selected item in the menu,
**				if menu handle is correct, NULL otherwise
**
**	Remarks		This functions gets current selected item of menu
**
*******************************************************************************/
BTE_HT_API extern tHT_MENU_ITEM* HT_GetMenuSelectedItem(UINT32 handle);

/*******************************************************************************
**
**	Function	HT_CreateEditBox
**
**	Parameters	flags - any combination of flags for edit box
**				p_prompt_text - title of edit box, for example
**					"Enter phone:"
**
**	Returns		handle unique to edit box object, or NULL if incorrect
**				parameters are used; function returns immediately.
**
**	Remarks		This function initializes an edit box object
**
*******************************************************************************/
BTE_HT_API extern UINT32 HT_CreateEditBox(UINT32 flags, UINT8 *p_prompt_text);

/*******************************************************************************
**
**	Function	HT_SetEditBoxText
**
**	Parameters	handle - UINT32 to previously created edit box
**				p_text - initial content of edit box. This parameter can be
**					 NULL
**
**	Returns		TRUE if valid handle is used, FALSE otherwise.
**
**	Remarks		This function sets the text in an edit box created earlier
**
*******************************************************************************/
BTE_HT_API extern BOOLEAN HT_SetEditBoxText(UINT32 handle, UINT8 *p_text);

/*******************************************************************************
**
**	Function	HT_SetEditBoxCursorPosition
**
**	Parameters	handle - previously created edit box
**				new_position - new cursor position on a symbol basis (from
**					the beginning of the string)
**
**	Returns		TRUE if parameters are correct, FALSE otherwise.
**
**	Remarks		This function changes the edit box cursor position.
**
*******************************************************************************/
BTE_HT_API extern BOOLEAN HT_SetEditBoxCursorPosition(UINT32 handle,
	UINT32 new_position);

/*******************************************************************************
**
**	Function	HT_GetEditBoxText
**
**	Parameters	handle - previously created edit box
**
**	Returns		pointer to text buffer. Warning! Do not edit buffer content!
**
**	Remarks		This function gets the text, currently being in indicated edit box.
**
*******************************************************************************/
BTE_HT_API extern UINT8* HT_GetEditBoxText(UINT32 handle);

/*******************************************************************************
**
**	Function	HT_MessageBoxTimeout
**
**	Parameters	p_text - text to display in message box
**				timeout - interval for which the message box will appear
**					in milliseconds
**
**	Returns		FALSE if some parameters are invalid, TRUE otherwise.
**
**	Remarks		This function displays a message box with text for some
**				interval that is passed as the second parameter. If the user
**				presses any key during this period, the message box closes immediately.
**
*******************************************************************************/
BTE_HT_API extern BOOLEAN HT_MessageBoxTimeout(UINT8 *p_text, UINT32 timeout);

/*******************************************************************************
**
**	Function	HT_MessageBox
**
**	Parameters	p_text - text to display in message box
**
**	Returns		handle unique to message box object, or NULL if incorrect
**				parameters are used.
**
**	Remarks		This function displays a message box with text.
**
*******************************************************************************/
BTE_HT_API extern UINT32 HT_MessageBox (UINT8 *p_text);

/*******************************************************************************
**
**	Function	HT_GetCurrMenuHandle
**
**	Parameters	void
**
**	Returns		handle to currently displayed menu
**
**	Remarks		This function displays a message box with text.
**
*******************************************************************************/
BTE_HT_API extern UINT32 HT_GetCurrMenuHandle (void);



#ifdef __cplusplus
}
#endif

#endif

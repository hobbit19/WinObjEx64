/*******************************************************************************
*
*  (C) COPYRIGHT AUTHORS, 2015 - 2018
*
*  TITLE:       PROPOBJECTDUMP.C
*
*  VERSION:     1.70
*
*  DATE:        30 Nov 2018
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
* ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED
* TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
*******************************************************************************/
#include "global.h"
#include "treelist\treelist.h"
#include "propDlg.h"
#include "propObjectDumpConsts.h"
#include "propTypeConsts.h"

//
// Global variables for treelist.
//
HWND g_TreeList;
ATOM g_TreeListAtom;

/*
* ObDumpShowError
*
* Purpose:
*
* Hide all windows for given hwnd and display error text.
*
*/
VOID ObDumpShowError(
    _In_ HWND hwndDlg
)
{
    RECT rGB;

    if (GetWindowRect(hwndDlg, &rGB)) {
        EnumChildWindows(hwndDlg, supEnumHideChildWindows, (LPARAM)&rGB);
    }
    ShowWindow(GetDlgItem(hwndDlg, ID_OBJECTDUMPERROR), SW_SHOW);
}

/*
* ObDumpShowMessage
*
* Purpose:
*
* Hide all windows for given hwnd and display message text.
*
*/
VOID ObDumpShowMessage(
    _In_ HWND hwndDlg,
    _In_ LPWSTR lpMessageText
)
{
    RECT rGB;

    if (GetWindowRect(hwndDlg, &rGB)) {
        EnumChildWindows(hwndDlg, supEnumHideChildWindows, (LPARAM)&rGB);
    }
    SetWindowText(GetDlgItem(hwndDlg, ID_OBJECTDUMPERROR), lpMessageText);
    ShowWindow(GetDlgItem(hwndDlg, ID_OBJECTDUMPERROR), SW_SHOW);
}

/*
* ObDumpAddress
*
* Purpose:
*
* Dump given Address to the treelist.
*
*/
VOID ObDumpAddress(
    _In_ HWND TreeList,
    _In_ HTREEITEM hParent,
    _In_ LPWSTR lpszName,
    _In_opt_ LPWSTR lpszDesc, //additional text to be displayed
    _In_opt_ PVOID Address,
    _In_ COLORREF BgColor,
    _In_ COLORREF FontColor
)
{
    TL_SUBITEMS_FIXED  subitems;
    WCHAR              szValue[100];

    RtlSecureZeroMemory(&subitems, sizeof(subitems));
    subitems.Count = 2;

    if (Address == NULL) {
        subitems.Text[0] = T_NULL;
    }
    else {
        RtlSecureZeroMemory(&szValue, sizeof(szValue));
        szValue[0] = L'0';
        szValue[1] = L'x';
        u64tohex((ULONG_PTR)Address, &szValue[2]);
        subitems.Text[0] = szValue;
    }
    if (lpszDesc) {
        if (BgColor != 0) {
            subitems.ColorFlags |= TLF_BGCOLOR_SET;
            subitems.BgColor = BgColor;
        }
        if (FontColor != 0) {
            subitems.ColorFlags |= TLF_FONTCOLOR_SET;
            subitems.FontColor = FontColor;
        }
        subitems.Text[1] = lpszDesc;
    }

    TreeListAddItem(
        TreeList,
        hParent,
        TVIF_TEXT | TVIF_STATE,
        0,
        0,
        lpszName,
        &subitems);
}

/*
* ObDumpAddressWithModule
*
* Purpose:
*
* Dump given Address to the treelist with module check.
*
*/
VOID ObDumpAddressWithModule(
    _In_ HTREEITEM hParent,
    _In_ LPWSTR lpszName,
    _In_opt_ PVOID Address,
    _In_ PRTL_PROCESS_MODULES pModules,
    _In_opt_ PVOID SelfDriverBase,
    _In_opt_ ULONG SelfDriverSize
)
{
    TL_SUBITEMS_FIXED   subitems;
    WCHAR               szValue[100], szModuleName[MAX_PATH * 2];

    RtlSecureZeroMemory(&subitems, sizeof(subitems));
    subitems.Count = 2;
    subitems.Text[0] = T_NULL;
    if (Address != NULL) {

        RtlSecureZeroMemory(&szValue, sizeof(szValue));
        szValue[0] = L'0';
        szValue[1] = L'x';
        u64tohex((ULONG_PTR)Address, &szValue[2]);
        subitems.Text[0] = szValue;

        RtlSecureZeroMemory(&szModuleName, sizeof(szModuleName));

        //if SelfDriverBase & SelfDriverSize present, look if Address routine points to current driver
        if (SelfDriverBase != NULL && SelfDriverSize) {
            if (!IN_REGION(Address, SelfDriverBase, SelfDriverSize)) {
                _strcpy(szModuleName, L"Hooked by ");
                subitems.ColorFlags = TLF_BGCOLOR_SET;
                subitems.BgColor = CLR_HOOK;
            }
        }
        if (supFindModuleNameByAddress(pModules, Address, _strend(szModuleName), MAX_PATH)) {
            subitems.Text[1] = szModuleName;
        }
        else {
            //unknown address outside any visible modules, warn
            subitems.Text[1] = T_Unknown;
            subitems.ColorFlags = TLF_BGCOLOR_SET;
            subitems.BgColor = CLR_WARN;
        }
    }

    TreeListAddItem(
        g_TreeList,
        hParent,
        TVIF_TEXT | TVIF_STATE,
        0,
        0,
        lpszName,
        &subitems);
}

/*
* ObDumpPushLock
*
* Purpose:
*
* Dump EX_PUSH_LOCK to the treelist.
*
*/
VOID ObDumpPushLock(
    _In_ HWND TreeList,
    _In_ HTREEITEM hParent,
    _In_ PVOID PushLockPtr,
    _In_opt_ COLORREF BgColor,
    _In_opt_ COLORREF FontColor
)
{
    TL_SUBITEMS_FIXED   subitems;
    HTREEITEM h_tviSubItem;

    RtlSecureZeroMemory(&subitems, sizeof(subitems));
    subitems.Count = 2;
    subitems.Text[1] = T_EX_PUSH_LOCK;

    h_tviSubItem = TreeListAddItem(
        TreeList,
        hParent,
        TVIF_TEXT | TVIF_STATE,
        0,
        0,
        TEXT("Lock"),
        &subitems);

    ObDumpAddress(TreeList, h_tviSubItem, TEXT("Ptr"), NULL, PushLockPtr, BgColor, FontColor);
}

/*
* ObDumpByte
*
* Purpose:
*
* Dump BYTE to the treelist.
* Dump BOOL if IsBool set.
* You must handle BOOLEAN differently.
*
*/
VOID ObDumpByte(
    _In_ HWND TreeList,
    _In_ HTREEITEM hParent,
    _In_ LPWSTR lpszName,
    _In_opt_ LPWSTR lpszDesc,
    _In_ BYTE Value,
    _In_opt_ COLORREF BgColor,
    _In_opt_ COLORREF FontColor,
    _In_ BOOL IsBool
)
{
    TL_SUBITEMS_FIXED   subitems;
    WCHAR               szValue[100];

    RtlSecureZeroMemory(&subitems, sizeof(subitems));

    subitems.Count = 1;
    if (lpszDesc != NULL) {
        subitems.Count = 2;
        subitems.Text[1] = lpszDesc;
    }

    RtlSecureZeroMemory(szValue, sizeof(szValue));
    if (IsBool) {
        _strcpy(szValue, (BOOL)(Value) ? L"TRUE" : L"FALSE");
    }
    else {
        wsprintf(szValue, FORMAT_HEXBYTE, Value);
    }

    subitems.Text[0] = szValue;

    if (BgColor != 0) {
        subitems.ColorFlags |= TLF_BGCOLOR_SET;
        subitems.BgColor = BgColor;
    }
    if (FontColor != 0) {
        subitems.ColorFlags |= TLF_FONTCOLOR_SET;
        subitems.FontColor = FontColor;
    }

    TreeListAddItem(
        TreeList,
        hParent,
        TVIF_TEXT | TVIF_STATE,
        0,
        0,
        lpszName,
        &subitems);
}

/*
* ObDumpSetString
*
* Purpose:
*
* Put string to the treelist.
*
*/
VOID ObDumpSetString(
    _In_ HWND TreeList,
    _In_ HTREEITEM hParent,
    _In_ LPWSTR lpszName,
    _In_opt_ LPWSTR lpszDesc,
    _In_ LPWSTR lpszValue,
    _In_opt_ COLORREF BgColor,
    _In_opt_ COLORREF FontColor
)
{
    TL_SUBITEMS_FIXED   subitems;

    RtlSecureZeroMemory(&subitems, sizeof(subitems));

    subitems.Count = 1;
    subitems.Text[0] = lpszValue;

    if (lpszDesc != NULL) {
        subitems.Count = 2;
        subitems.Text[1] = lpszDesc;
    }

    if (BgColor != 0) {
        subitems.ColorFlags |= TLF_BGCOLOR_SET;
        subitems.BgColor = BgColor;
    }
    if (FontColor != 0) {
        subitems.ColorFlags |= TLF_FONTCOLOR_SET;
        subitems.FontColor = FontColor;
    }

    TreeListAddItem(
        TreeList,
        hParent,
        TVIF_TEXT | TVIF_STATE,
        0,
        0,
        lpszName,
        &subitems);
}

/*
* ObDumpUlong
*
* Purpose:
*
* Dump ULONG 4 bytes / USHORT 2 bytes to the treelist.
*
*/
VOID ObDumpUlong(
    _In_ HWND TreeList,
    _In_ HTREEITEM hParent,
    _In_ LPWSTR lpszName,
    _In_opt_ LPWSTR lpszDesc, //additional text to be displayed
    _In_ ULONG Value,
    _In_ BOOL HexDump,
    _In_ BOOL IsUShort,
    _In_opt_ COLORREF BgColor,
    _In_opt_ COLORREF FontColor
)
{
    TL_SUBITEMS_FIXED   subitems;
    WCHAR               szValue[100];

    RtlSecureZeroMemory(&szValue, sizeof(szValue));
    RtlSecureZeroMemory(&subitems, sizeof(subitems));

    if (lpszDesc != NULL) {
        subitems.Count = 2;
        subitems.Text[1] = lpszDesc;
    }
    else {
        subitems.Count = 1;
    }

    if (HexDump) {
        if (IsUShort) {
            wsprintf(szValue, FORMAT_HEXUSHORT, Value);
        }
        else {
            szValue[0] = L'0';
            szValue[1] = L'x';
            ultohex(Value, &szValue[2]);
        }
    }
    else {
        if (IsUShort) {
            wsprintf(szValue, FORMAT_USHORT, Value);
        }
        else {
            ultostr(Value, szValue);
        }
    }
    subitems.Text[0] = szValue;

    if (BgColor != 0) {
        subitems.ColorFlags |= TLF_BGCOLOR_SET;
        subitems.BgColor = BgColor;
    }
    if (FontColor != 0) {
        subitems.ColorFlags |= TLF_FONTCOLOR_SET;
        subitems.FontColor = FontColor;
    }

    TreeListAddItem(
        TreeList,
        hParent,
        TVIF_TEXT | TVIF_STATE,
        0,
        0,
        lpszName,
        &subitems);
}

/*
* ObDumpUlong64
*
* Purpose:
*
* Dump ULONG 8 byte to the treelist.
*
*/
VOID ObDumpUlong64(
    _In_ HWND TreeList,
    _In_ HTREEITEM hParent,
    _In_ LPWSTR lpszName,
    _In_opt_ LPWSTR lpszDesc, //additional text to be displayed
    _In_opt_ ULONG64 Value,
    _In_ BOOL OutAsHex,
    _In_ COLORREF BgColor,
    _In_ COLORREF FontColor
)
{
    TL_SUBITEMS_FIXED  subitems;
    WCHAR              szValue[100];

    RtlSecureZeroMemory(&subitems, sizeof(subitems));
    subitems.Count = 2;

    RtlSecureZeroMemory(&szValue, sizeof(szValue));

    if (OutAsHex) {
        szValue[0] = TEXT('0');
        szValue[1] = TEXT('x');
        u64tohex(Value, &szValue[2]);
    }
    else {
        u64tostr(Value, szValue);
    }
    subitems.Text[0] = szValue;

    if (lpszDesc) {
        if (BgColor != 0) {
            subitems.ColorFlags |= TLF_BGCOLOR_SET;
            subitems.BgColor = BgColor;
        }
        if (FontColor != 0) {
            subitems.ColorFlags |= TLF_FONTCOLOR_SET;
            subitems.FontColor = FontColor;
        }
        subitems.Text[1] = lpszDesc;
    }

    TreeListAddItem(
        TreeList,
        hParent,
        TVIF_TEXT | TVIF_STATE,
        0,
        0,
        lpszName,
        &subitems);
}

/*
* ObDumpULargeInteger
*
* Purpose:
*
* Dump ULARGE_INTEGER members to the treelist.
*
*/
VOID ObDumpULargeInteger(
    _In_ HWND TreeList,
    _In_ HTREEITEM hParent,
    _In_ LPWSTR ListEntryName,
    _In_opt_ PULARGE_INTEGER Value
)
{
    HTREEITEM           h_tviSubItem;
    TL_SUBITEMS_FIXED   subitems;
    WCHAR               szValue[100];

    h_tviSubItem = TreeListAddItem(
        TreeList,
        hParent,
        TVIF_TEXT | TVIF_STATE,
        TVIS_EXPANDED,
        0,
        ListEntryName,
        NULL);

    if (h_tviSubItem == NULL) {
        return;
    }

    //add large integer entry item to treelist and exit if value is null
    if (Value == NULL) {
        return;
    }

    RtlSecureZeroMemory(&subitems, sizeof(subitems));
    subitems.Count = 1;

    RtlSecureZeroMemory(&szValue, sizeof(szValue));
    szValue[0] = L'0';
    szValue[1] = L'x';
    ultohex(Value->LowPart, &szValue[2]);
    subitems.Text[0] = szValue;

    TreeListAddItem(
        TreeList,
        h_tviSubItem,
        TVIF_TEXT | TVIF_STATE,
        TVIS_EXPANDED,
        0,
        L"LowPart",
        &subitems);

    RtlSecureZeroMemory(&szValue, sizeof(szValue));
    szValue[0] = L'0';
    szValue[1] = L'x';
    ultohex(Value->HighPart, &szValue[2]);
    subitems.Text[0] = szValue;

    TreeListAddItem(
        TreeList,
        h_tviSubItem,
        TVIF_TEXT | TVIF_STATE,
        TVIS_EXPANDED,
        0,
        L"HighPart",
        &subitems);
}

/*
* ObDumpListEntry
*
* Purpose:
*
* Dump LIST_ENTRY members to the treelist.
*
*/
VOID ObDumpListEntry(
    _In_ HWND TreeList,
    _In_ HTREEITEM hParent,
    _In_ LPWSTR ListEntryName,
    _In_opt_ PLIST_ENTRY ListEntry
)
{
    HTREEITEM           h_tviSubItem;
    TL_SUBITEMS_FIXED   subitems;
    WCHAR               szValue[100];

    h_tviSubItem = TreeListAddItem(
        TreeList,
        hParent,
        TVIF_TEXT | TVIF_STATE,
        TVIS_EXPANDED,
        0,
        ListEntryName,
        NULL);

    if (h_tviSubItem == NULL) {
        return;
    }

    //add list entry item to treelist and exit if listentry is null
    if (ListEntry == NULL) {
        return;
    }

    RtlSecureZeroMemory(&subitems, sizeof(subitems));
    subitems.Count = 1;

    if (ListEntry->Flink == NULL) {
        subitems.Text[0] = T_NULL;
    }
    else {
        RtlSecureZeroMemory(&szValue, sizeof(szValue));
        szValue[0] = L'0';
        szValue[1] = L'x';
        u64tohex((ULONG_PTR)ListEntry->Flink, &szValue[2]);
        subitems.Text[0] = szValue;
    }

    TreeListAddItem(
        TreeList,
        h_tviSubItem,
        TVIF_TEXT | TVIF_STATE,
        TVIS_EXPANDED,
        0,
        L"Flink",
        &subitems);

    if (ListEntry->Blink == NULL) {
        subitems.Text[0] = T_NULL;
    }
    else {
        RtlSecureZeroMemory(&szValue, sizeof(szValue));
        szValue[0] = L'0';
        szValue[1] = L'x';
        u64tohex((ULONG_PTR)ListEntry->Blink, &szValue[2]);
        subitems.Text[0] = szValue;
    }

    TreeListAddItem(
        TreeList,
        h_tviSubItem,
        TVIF_TEXT | TVIF_STATE,
        0,
        0,
        L"Blink",
        &subitems);
}

/*
* ObDumpUnicodeString
*
* Purpose:
*
* Dump UNICODE_STRING members to the treelist.
* Support PUNICODE_STRING, address must point to kernel memory.
*
*/
VOID ObDumpUnicodeString(
    _In_ HTREEITEM hParent,
    _In_ LPWSTR StringName,
    _In_opt_ PUNICODE_STRING pString,
    _In_ BOOL NeedDump
)
{
    LPWSTR              lpObjectName;
    HTREEITEM           h_tviSubItem;
    TL_SUBITEMS_FIXED   subitems;
    UNICODE_STRING      uStr;
    WCHAR               szValue[100];

    RtlSecureZeroMemory(&uStr, sizeof(uStr));
    RtlSecureZeroMemory(&subitems, sizeof(subitems));
    subitems.Count = 2;

    //add root entry
    //if pString points to kernel mode address, dump it, otherwise simple copy
    if (NeedDump) {
        //check if NULL, add entry
        if (pString == NULL) {
            subitems.Text[0] = T_NULL;
        }
        else {
            //pString->Buffer need to be dumped
            RtlSecureZeroMemory(&szValue, sizeof(szValue));
            szValue[0] = L'0';
            szValue[1] = L'x';
            u64tohex((ULONG_PTR)pString, &szValue[2]);
            subitems.Text[0] = szValue;
            subitems.Text[1] = T_PUNICODE_STRING;
            kdReadSystemMemoryEx((ULONG_PTR)pString, &uStr, sizeof(UNICODE_STRING), NULL);
        }
    }
    else {
        if (pString) {
            uStr.Buffer = pString->Buffer;
            uStr.Length = pString->Length;
            uStr.MaximumLength = pString->MaximumLength;
        }
        else {
            uStr.Buffer = NULL;
            uStr.Length = 0;
            uStr.MaximumLength = 0;
        }
    }

    h_tviSubItem = TreeListAddItem(
        g_TreeList,
        hParent,
        TVIF_TEXT | TVIF_STATE,
        TVIS_EXPANDED,
        0,
        StringName,
        NeedDump ? &subitems : NULL);

    //string points to nowhere, only root entry added
    if (pString == NULL) {
        return;
    }

    //
    //UNICODE_STRING.Length
    //
    RtlSecureZeroMemory(&subitems, sizeof(subitems));
    RtlSecureZeroMemory(&szValue, sizeof(szValue));
    wsprintf(szValue, FORMAT_HEXUSHORT, uStr.Length);
    subitems.Count = 2;
    subitems.Text[0] = szValue;

    TreeListAddItem(
        g_TreeList,
        h_tviSubItem,
        TVIF_TEXT | TVIF_STATE,
        0,
        0,
        T_LENGTH,
        &subitems);

    //
    //UNICODE_STRING.MaximumLength
    //
    RtlSecureZeroMemory(&szValue, sizeof(szValue));
    RtlSecureZeroMemory(&subitems, sizeof(subitems));
    wsprintf(szValue, FORMAT_HEXUSHORT, uStr.MaximumLength);
    subitems.Count = 2;
    subitems.Text[0] = szValue;

    TreeListAddItem(
        g_TreeList,
        h_tviSubItem,
        TVIF_TEXT | TVIF_STATE,
        0,
        0,
        L"MaximumLength",
        &subitems);

    //
    //UNICODE_STRING.Buffer
    //
    RtlSecureZeroMemory(&subitems, sizeof(subitems));
    subitems.Count = 2;

    lpObjectName = NULL;
    if (uStr.Buffer == NULL) {
        subitems.Text[0] = T_NULL;
    }
    else {
        RtlSecureZeroMemory(&szValue, sizeof(szValue));
        szValue[0] = L'0';
        szValue[1] = L'x';
        u64tohex((ULONG_PTR)uStr.Buffer, &szValue[2]);
        subitems.Text[0] = szValue;

        //dump unicode string buffer
        lpObjectName = (LPWSTR)supHeapAlloc(uStr.Length + sizeof(UNICODE_NULL));
        if (lpObjectName) {

            kdReadSystemMemoryEx(
                (ULONG_PTR)uStr.Buffer,
                lpObjectName,
                uStr.Length,
                NULL);

        }
        subitems.Text[1] = lpObjectName;
    }

    TreeListAddItem(
        g_TreeList,
        h_tviSubItem,
        TVIF_TEXT | TVIF_STATE,
        0,
        0,
        L"Buffer",
        &subitems);

    if (lpObjectName) {
        supHeapFree(lpObjectName);
    }
}

/*
* ObDumpDispatcherHeader
*
* Purpose:
*
* Dump DISPATCHER_HEADER members to the treelist.
*
*/
VOID ObDumpDispatcherHeader(
    _In_ HTREEITEM hParent,
    _In_ DISPATCHER_HEADER *Header,
    _In_opt_ LPWSTR lpDescType,
    _In_opt_ LPWSTR lpDescSignalState,
    _In_opt_ LPWSTR lpDescSize
)
{
    HTREEITEM h_tviSubItem;

    h_tviSubItem = TreeListAddItem(
        g_TreeList,
        hParent,
        TVIF_TEXT | TVIF_STATE,
        0,
        0,
        L"Header",
        NULL);

    if (h_tviSubItem) {

        //Header->Type
        ObDumpUlong(g_TreeList, h_tviSubItem, L"Type", lpDescType, Header->Type, TRUE, TRUE, 0, 0);
        //Header->Absolute
        ObDumpUlong(g_TreeList, h_tviSubItem, L"Absolute", NULL, Header->Absolute, TRUE, TRUE, 0, 0);
        //Header->Size
        ObDumpUlong(g_TreeList, h_tviSubItem, L"Size", lpDescSize, Header->Size, TRUE, TRUE, 0, 0);
        //Header->Inserted
        ObDumpByte(g_TreeList, h_tviSubItem, L"Inserted", NULL, Header->Inserted, 0, 0, TRUE);
        //Header->SignalState
        ObDumpUlong(g_TreeList, h_tviSubItem, L"SignalState", lpDescSignalState, Header->SignalState, TRUE, FALSE, 0, 0);
        //Header->WaitListHead
        ObDumpListEntry(g_TreeList, h_tviSubItem, L"WaitListHead", &Header->WaitListHead);
    }
}

/*
* ObDumpSqos
*
* Purpose:
*
* Dump SECURITY_QUALITY_OF_SERVICE to the treelist.
*
*/
VOID ObDumpSqos(
    _In_ HWND TreeList,
    _In_ HTREEITEM hParent,
    _In_ SECURITY_QUALITY_OF_SERVICE *SecurityQos
)
{
    LPWSTR lpType;
    HTREEITEM h_tviSubItem;
    TL_SUBITEMS_FIXED subitems;

    RtlSecureZeroMemory(&subitems, sizeof(subitems));
    subitems.Count = 2;
    subitems.Text[1] = TEXT("SECURITY_QUALITY_OF_SERVICE");

    h_tviSubItem = TreeListAddItem(
        TreeList,
        hParent,
        TVIF_TEXT,
        0,
        0,
        TEXT("SecurityQos"),
        &subitems);

    ObDumpUlong(
        TreeList,
        h_tviSubItem,
        TEXT("Length"),
        NULL,
        SecurityQos->Length,
        TRUE,
        FALSE,
        0,
        0);

    switch (SecurityQos->ImpersonationLevel) {
    case SecurityIdentification:
        lpType = TEXT("SecurityIdentification");
        break;
    case SecurityImpersonation:
        lpType = TEXT("SecurityImpersonation");
        break;
    case SecurityDelegation:
        lpType = TEXT("SecurityDelegation");
        break;
    case SecurityAnonymous:
        lpType = TEXT("SecurityAnonymous");
        break;
    default:
        lpType = T_UnknownType;
        break;
    }

    ObDumpUlong(
        TreeList,
        h_tviSubItem,
        TEXT("ImpersonationLevel"),
        lpType,
        SecurityQos->ImpersonationLevel,
        FALSE,
        FALSE,
        0,
        0);

    if (SecurityQos->ContextTrackingMode)
        lpType = TEXT("SECURITY_DYNAMIC_TRACKING");
    else
        lpType = TEXT("SECURITY_STATIC_TRACKING");

    ObDumpByte(
        TreeList,
        h_tviSubItem,
        TEXT("ContextTrackingMode"),
        lpType,
        SecurityQos->ContextTrackingMode,
        0,
        0,
        TRUE);

    ObDumpByte(
        g_TreeList,
        h_tviSubItem,
        TEXT("EffectiveOnly"),
        NULL,
        SecurityQos->EffectiveOnly,
        0,
        0,
        TRUE);
}

/*
* ObDumpDriverObject
*
* Purpose:
*
* Dump DRIVER_OBJECT members to the treelist.
*
*/
VOID ObDumpDriverObject(
    _In_ PROP_OBJECT_INFO *Context,
    _In_ HWND hwndDlg
)
{
    BOOL                    cond, bOkay;
    INT                     i, j;
    HTREEITEM               h_tviRootItem, h_tviSubItem;
    PRTL_PROCESS_MODULES    pModules;
    PVOID                   pObj;
    POBJREF                 LookupObject;
    LPWSTR                  lpType;
    DRIVER_OBJECT           drvObject;
    DRIVER_EXTENSION        drvExtension;
    FAST_IO_DISPATCH        fastIoDispatch;
    LDR_DATA_TABLE_ENTRY    ldrEntry, ntosEntry;
    TL_SUBITEMS_FIXED       subitems;
    COLORREF                BgColor;
    WCHAR                   szValue1[MAX_PATH + 1];

    if (Context == NULL) {
        return;
    }

    bOkay = FALSE;
    cond = FALSE;

    __try {

        RtlSecureZeroMemory(&drvObject, sizeof(drvObject));
        RtlSecureZeroMemory(&ldrEntry, sizeof(ldrEntry));

        do {

            //dump drvObject
            if (!kdReadSystemMemoryEx(
                Context->ObjectInfo.ObjectAddress,
                &drvObject,
                sizeof(drvObject),
                NULL))
            {
                break;
            }

            //we need to dump drvObject
            //consider dump failures for anything else as not critical
            bOkay = TRUE;

            //dump drvObject->DriverSection
            if (!kdReadSystemMemoryEx(
                (ULONG_PTR)drvObject.DriverSection,
                &ldrEntry,
                sizeof(ldrEntry),
                NULL))
            {
                break;
            }

        } while (cond);

        //any errors - abort
        if (!bOkay) {
            ObDumpShowError(hwndDlg);
            return;
        }

        g_TreeList = 0;
        g_TreeListAtom = 0;
        if (!supInitTreeListForDump(hwndDlg, &g_TreeListAtom, &g_TreeList)) {
            ObDumpShowError(hwndDlg);
            return;
        }

        //
        //DRIVER_OBJECT
        //

        h_tviRootItem = TreeListAddItem(
            g_TreeList,
            NULL,
            TVIF_TEXT | TVIF_STATE,
            TVIS_EXPANDED,
            TVIS_EXPANDED,
            TEXT("DRIVER_OBJECT"),
            NULL);

        //Type
        BgColor = 0;
        lpType = TEXT("IO_TYPE_DRIVER");
        if (drvObject.Type != IO_TYPE_DRIVER) {
            lpType = TEXT("! Must be IO_TYPE_DRIVER");
            BgColor = CLR_WARN;
        }
        ObDumpUlong(g_TreeList, h_tviRootItem, TEXT("Type"), lpType, drvObject.Type, TRUE, TRUE, BgColor, 0);

        //Size
        BgColor = 0;
        lpType = NULL;
        if (drvObject.Size != sizeof(DRIVER_OBJECT)) {
            lpType = TEXT("! Must be sizeof(DRIVER_OBJECT)");
            BgColor = CLR_WARN;
        }
        ObDumpUlong(g_TreeList, h_tviRootItem, TEXT("Size"), lpType, drvObject.Size, TRUE, TRUE, BgColor, 0);

        //DeviceObject
        lpType = NULL;
        BgColor = 0;
        if (drvObject.DeviceObject != NULL) {

            LookupObject = ObCollectionFindByAddress(
                &g_kdctx.ObCollection,
                (ULONG_PTR)drvObject.DeviceObject,
                FALSE);

            if (LookupObject != NULL) {
                lpType = LookupObject->ObjectName;
            }
            else {
                lpType = T_UNNAMED;
                BgColor = CLR_LGRY;
            }
        }
        ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("DeviceObject"), lpType, drvObject.DeviceObject, BgColor, 0);

        //Flags
        RtlSecureZeroMemory(&szValue1, sizeof(szValue1));
        RtlSecureZeroMemory(&subitems, sizeof(subitems));
        j = 0;
        lpType = NULL;
        if (drvObject.Flags) {
            for (i = 0; i < MAX_KNOWN_DRV_FLAGS; i++) {
                if (drvObject.Flags & drvFlags[i].dwValue) {
                    lpType = drvFlags[i].lpDescription;
                    subitems.Count = 2;

                    //add first entry with name
                    if (j == 0) {
                        szValue1[0] = L'0';
                        szValue1[1] = L'x';
                        ultohex(drvObject.Flags, &szValue1[2]);

                        subitems.Text[0] = szValue1;
                        subitems.Text[1] = lpType;
                    }
                    else {
                        //add subentry
                        subitems.Text[0] = NULL;
                        subitems.Text[1] = lpType;
                    }

                    TreeListAddItem(
                        g_TreeList,
                        h_tviRootItem,
                        TVIF_TEXT | TVIF_STATE,
                        0,
                        0,
                        (j == 0) ? T_FLAGS : NULL,
                        &subitems);

                    drvObject.Flags &= ~drvFlags[i].dwValue;
                    j++;
                }
                if (drvObject.Flags == 0) {
                    break;
                }
            }
        }
        else {
            //add named entry with zero data
            ObDumpUlong(g_TreeList, h_tviRootItem, T_FLAGS, NULL, 0, TRUE, FALSE, 0, 0);
        }

        //DriverStart
        ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("DriverStart"), NULL, drvObject.DriverStart, 0, 0);

        //DriverSection
        ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("DriverSection"), TEXT("PLDR_DATA_TABLE_ENTRY"), drvObject.DriverSection, 0, 0);

        //DriverExtension
        ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("DriverExtension"), TEXT("PDRIVER_EXTENSION"), drvObject.DriverExtension, 0, 0);

        //DriverName
        ObDumpUnicodeString(h_tviRootItem, TEXT("DriverName"), &drvObject.DriverName, FALSE);

        //HardwareDatabase
        ObDumpUnicodeString(h_tviRootItem, TEXT("HardwareDatabase"), drvObject.HardwareDatabase, TRUE);

        //FastIoDispatch
        ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("FastIoDispatch"), TEXT("PFAST_IO_DISPATCH"), drvObject.FastIoDispatch, 0, 0);

        //DriverInit
        ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("DriverInit"), NULL, drvObject.DriverInit, 0, 0);

        //DriverStartIo
        ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("DriverStartIo"), NULL, drvObject.DriverStartIo, 0, 0);

        //DriverUnload
        ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("DriverUnload"), NULL, drvObject.DriverUnload, 0, 0);

        //MajorFunction
        RtlSecureZeroMemory(&szValue1, sizeof(szValue1));
        RtlSecureZeroMemory(&subitems, sizeof(subitems));
        subitems.Count = 2;
        subitems.Text[0] = TEXT("{...}");
        subitems.Text[1] = NULL;

        h_tviSubItem = TreeListAddItem(
            g_TreeList,
            h_tviRootItem,
            TVIF_TEXT | TVIF_STATE,
            0,
            0,
            TEXT("MajorFunction"),
            &subitems);

        RtlSecureZeroMemory(&ntosEntry, sizeof(ntosEntry));
        pModules = (PRTL_PROCESS_MODULES)supGetSystemInfo(SystemModuleInformation);

        if (g_kdctx.IopInvalidDeviceRequest == NULL)
            g_kdctx.IopInvalidDeviceRequest = kdQueryIopInvalidDeviceRequest();

        for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {

            if (drvObject.MajorFunction[i] == NULL) {
                continue;
            }

            //skip ntoskrnl default irp handler
            //warning may skip actual trampoline hook
            if (g_kdctx.IopInvalidDeviceRequest) {
                if ((ULONG_PTR)drvObject.MajorFunction[i] == (ULONG_PTR)g_kdctx.IopInvalidDeviceRequest) {

                    ObDumpAddress(
                        g_TreeList,
                        h_tviSubItem,
                        T_IRP_MJ_FUNCTION[i],
                        TEXT("nt!IopInvalidDeviceRequest"),
                        drvObject.MajorFunction[i],
                        CLR_INVL,
                        0);

                    continue;
                }
            }

            //DRIVER_OBJECT->MajorFunction[i]
            ObDumpAddressWithModule(h_tviSubItem, T_IRP_MJ_FUNCTION[i], drvObject.MajorFunction[i],
                pModules, ldrEntry.DllBase, ldrEntry.SizeOfImage);
        }

        //
        //LDR_DATA_TABLE_ENTRY
        //

        if (drvObject.DriverSection != NULL) {

            //root itself
            h_tviRootItem = TreeListAddItem(
                g_TreeList,
                NULL,
                TVIF_TEXT | TVIF_STATE,
                TVIS_EXPANDED,
                0,
                T_LDR_DATA_TABLE_ENTRY,
                NULL);

            //InLoadOrderLinks
            ObDumpListEntry(g_TreeList, h_tviRootItem, TEXT("InLoadOrderLinks"), &ldrEntry.InLoadOrderLinks);

            //InMemoryOrderLinks
            ObDumpListEntry(g_TreeList, h_tviRootItem, TEXT("InMemoryOrderLinks"), &ldrEntry.InMemoryOrderLinks);

            //InInitializationOrderLinks/InProgressLinks
            lpType = TEXT("InInitializationOrderLinks");
            if (g_NtBuildNumber >= 9600) {
                lpType = TEXT("InProgressLinks");
            }
            ObDumpListEntry(g_TreeList, h_tviRootItem, lpType, &ldrEntry.DUMMYUNION0.InInitializationOrderLinks);

            //DllBase
            ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("DllBase"), NULL, ldrEntry.DllBase, 0, 0);

            //EntryPoint
            ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("EntryPoint"), NULL, ldrEntry.EntryPoint, 0, 0);

            //SizeOfImage
            ObDumpUlong(g_TreeList, h_tviRootItem, TEXT("SizeOfImage"), NULL, ldrEntry.SizeOfImage, TRUE, FALSE, 0, 0);

            //FullDllName
            ObDumpUnicodeString(h_tviRootItem, TEXT("FullDllName"), &ldrEntry.FullDllName, FALSE);

            //BaseDllName
            ObDumpUnicodeString(h_tviRootItem, TEXT("BaseDllName"), &ldrEntry.BaseDllName, FALSE);

            //Flags
            ObDumpUlong(g_TreeList, h_tviRootItem, T_FLAGS, NULL, ldrEntry.ENTRYFLAGSUNION.Flags, TRUE, FALSE, 0, 0);

            //LoadCount
            lpType = TEXT("ObsoleteLoadCount");
            if (g_NtBuildNumber < 9200) {
                lpType = TEXT("LoadCount");
            }
            ObDumpUlong(g_TreeList, h_tviRootItem, lpType, NULL, ldrEntry.ObsoleteLoadCount, TRUE, TRUE, 0, 0);

            //TlsIndex
            ObDumpUlong(g_TreeList, h_tviRootItem, TEXT("TlsIndex"), NULL, ldrEntry.TlsIndex, TRUE, TRUE, 0, 0);

            //SectionPointer
            ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("SectionPointer"), NULL, ldrEntry.DUMMYUNION1.SectionPointer, 0, 0);

            //CheckSum
            ObDumpUlong(g_TreeList, h_tviRootItem, TEXT("CheckSum"), NULL, ldrEntry.DUMMYUNION1.CheckSum, TRUE, FALSE, 0, 0);

            //LoadedImports
            if (g_NtBuildNumber < 9200) {
                ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("LoadedImports"), NULL, ldrEntry.DUMMYUNION2.LoadedImports, 0, 0);
            }

        } //LDR_DATA_TABLE_ENTRY


        //
        //FAST_IO_DISPATCH
        //

        if (drvObject.FastIoDispatch != NULL) {

            RtlSecureZeroMemory(&fastIoDispatch, sizeof(fastIoDispatch));

            if (kdReadSystemMemoryEx(
                (ULONG_PTR)drvObject.FastIoDispatch,
                &fastIoDispatch,
                sizeof(fastIoDispatch),
                NULL))
            {

                h_tviRootItem = TreeListAddItem(
                    g_TreeList,
                    NULL,
                    TVIF_TEXT | TVIF_STATE,
                    0,
                    0,
                    TEXT("FAST_IO_DISPATCH"),
                    NULL);

                //SizeOfFastIoDispatch
                BgColor = 0;
                lpType = NULL;
                bOkay = TRUE;
                if (fastIoDispatch.SizeOfFastIoDispatch != sizeof(FAST_IO_DISPATCH)) {
                    lpType = TEXT("! Must be sizeof(FAST_IO_DISPATCH)");
                    BgColor = CLR_WARN;
                    bOkay = FALSE;//<-set flag invalid structure
                }
                ObDumpUlong(g_TreeList, h_tviRootItem, TEXT("SizeOfFastIoDispatch"), lpType, fastIoDispatch.SizeOfFastIoDispatch, TRUE, FALSE, BgColor, 0);

                //valid structure
                if (bOkay) {
                    for (i = 0; i < 27; i++) {
                        pObj = ((PVOID *)(&fastIoDispatch.FastIoCheckIfPossible))[i];
                        if (pObj == NULL) {
                            continue;
                        }
                        ObDumpAddressWithModule(h_tviRootItem, T_FAST_IO_DISPATCH[i], pObj,
                            pModules, ldrEntry.DllBase, ldrEntry.SizeOfImage);
                    }
                }

            } //kdReadSystemMemoryEx
        } //if

        //
        //PDRIVER_EXTENSION
        //
        if (drvObject.DriverExtension != NULL) {
            //dump drvObject->DriverExtension

            RtlSecureZeroMemory(&drvExtension, sizeof(drvExtension));

            if (kdReadSystemMemoryEx(
                (ULONG_PTR)drvObject.DriverExtension,
                &drvExtension,
                sizeof(drvExtension),
                NULL))
            {

                h_tviRootItem = TreeListAddItem(
                    g_TreeList,
                    NULL,
                    TVIF_TEXT | TVIF_STATE,
                    TVIS_EXPANDED,
                    0,
                    TEXT("DRIVER_EXTENSION"),
                    NULL);

                //DriverObject
                BgColor = 0;
                lpType = NULL;

                //must be self-ref
                if ((ULONG_PTR)drvExtension.DriverObject != (ULONG_PTR)Context->ObjectInfo.ObjectAddress) {
                    lpType = TEXT("! Bad DRIVER_OBJECT");
                    BgColor = CLR_WARN;
                }
                else {
                    //find ref
                    LookupObject = ObCollectionFindByAddress(
                        &g_kdctx.ObCollection,
                        (ULONG_PTR)drvExtension.DriverObject,
                        FALSE);

                    if (LookupObject != NULL) {
                        lpType = LookupObject->ObjectName;
                    }
                    else {
                        //sef-ref not found, notify, could be object outside directory so we don't know it name etc
                        lpType = T_REFNOTFOUND;
                        BgColor = CLR_INVL;
                    }
                }

                ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("DriverObject"), lpType, drvExtension.DriverObject, BgColor, 0);

                //AddDevice
                ObDumpAddressWithModule(h_tviRootItem, TEXT("AddDevice"), drvExtension.AddDevice,
                    pModules, ldrEntry.DllBase, ldrEntry.SizeOfImage);

                //Count
                ObDumpUlong(g_TreeList, h_tviRootItem, TEXT("Count"), NULL, drvExtension.Count, FALSE, FALSE, 0, 0);

                //ServiceKeyName
                ObDumpUnicodeString(h_tviRootItem, TEXT("ServiceKeyName"), &drvExtension.ServiceKeyName, FALSE);
            }
        }
        //
        //Cleanup
        //
        if (pModules) {
            supHeapFree(pModules);
        }

    }
    __except (exceptFilter(GetExceptionCode(), GetExceptionInformation())) {
        return;
    }
}

/*
* ObDumpDeviceObject
*
* Purpose:
*
* Dump DEVICE_OBJECT members to the treelist.
*
*/
VOID ObDumpDeviceObject(
    _In_ PROP_OBJECT_INFO *Context,
    _In_ HWND hwndDlg
)
{
    BOOL                    bOkay;
    INT                     i, j;
    HTREEITEM               h_tviRootItem, h_tviWcb, h_tviSubItem, h_tviWaitEntry;
    POBJREF                 LookupObject;
    LPWSTR                  lpType;
    TL_SUBITEMS_FIXED       subitems;
    DEVICE_OBJECT           devObject;
    DEVOBJ_EXTENSION        devObjExt;
    COLORREF                BgColor;
    WCHAR                   szValue1[MAX_PATH + 1];

    if (Context == NULL) {
        return;
    }

    bOkay = FALSE;

    __try {

        //dump devObject
        RtlSecureZeroMemory(&devObject, sizeof(devObject));

        if (!kdReadSystemMemoryEx(
            Context->ObjectInfo.ObjectAddress,
            &devObject,
            sizeof(devObject),
            NULL))
        {
            ObDumpShowError(hwndDlg);
            return;
        }

        g_TreeList = 0;
        g_TreeListAtom = 0;
        if (!supInitTreeListForDump(hwndDlg, &g_TreeListAtom, &g_TreeList)) {
            ObDumpShowError(hwndDlg);
            return;
        }

        //
        //DEVICE_OBJECT
        //

        h_tviRootItem = TreeListAddItem(g_TreeList, NULL, TVIF_TEXT | TVIF_STATE, TVIS_EXPANDED,
            TVIS_EXPANDED, L"DEVICE_OBJECT", NULL);

        //Type
        BgColor = 0;
        lpType = L"IO_TYPE_DEVICE";
        if (devObject.Type != IO_TYPE_DEVICE) {
            lpType = L"! Must be IO_TYPE_DEVICE";
            BgColor = CLR_WARN;
        }
        ObDumpUlong(g_TreeList, h_tviRootItem, L"Type", lpType, devObject.Type, TRUE, TRUE, BgColor, 0);

        //Size
        ObDumpUlong(g_TreeList, h_tviRootItem, L"Size", NULL, devObject.Size, TRUE, TRUE, 0, 0);

        //ReferenceCount
        ObDumpUlong(g_TreeList, h_tviRootItem, L"ReferenceCount", NULL, devObject.ReferenceCount, FALSE, FALSE, 0, 0);

        //DriverObject
        lpType = NULL;
        BgColor = 0;

        LookupObject = ObCollectionFindByAddress(
            &g_kdctx.ObCollection,
            (ULONG_PTR)devObject.DriverObject,
            FALSE);

        if (LookupObject != NULL) {
            lpType = LookupObject->ObjectName;
        }
        else {
            lpType = T_REFNOTFOUND;
            BgColor = CLR_INVL; //object can be outside directory so we don't know about it
        }
        ObDumpAddress(g_TreeList, h_tviRootItem, L"DriverObject", lpType, devObject.DriverObject, BgColor, 0);

        //NextDevice
        lpType = NULL;

        LookupObject = ObCollectionFindByAddress(
            &g_kdctx.ObCollection,
            (ULONG_PTR)devObject.NextDevice,
            FALSE);

        if (LookupObject != NULL) {
            lpType = LookupObject->ObjectName;
        }
        else {
            lpType = NULL;
        }
        ObDumpAddress(g_TreeList, h_tviRootItem, L"NextDevice", lpType, devObject.NextDevice, 0, 0);

        //AttachedDevice
        lpType = NULL;

        LookupObject = ObCollectionFindByAddress(
            &g_kdctx.ObCollection,
            (ULONG_PTR)devObject.AttachedDevice,
            FALSE);

        if (LookupObject != NULL) {
            lpType = LookupObject->ObjectName;
        }
        else {
            lpType = NULL;
        }
        ObDumpAddress(g_TreeList, h_tviRootItem, L"AttachedDevice", lpType, devObject.AttachedDevice, 0, 0);

        //CurrentIrp
        ObDumpAddress(g_TreeList, h_tviRootItem, L"CurrentIrp", NULL, devObject.CurrentIrp, 0, 0);

        //Timer
        lpType = L"PIO_TIMER";
        ObDumpAddress(g_TreeList, h_tviRootItem, L"Timer", lpType, devObject.Timer, 0, 0);

        //Flags
        RtlSecureZeroMemory(&szValue1, sizeof(szValue1));
        RtlSecureZeroMemory(&subitems, sizeof(subitems));
        lpType = NULL;
        j = 0;
        if (devObject.Flags) {
            for (i = 0; i < MAX_KNOWN_DEV_FLAGS; i++) {
                if (devObject.Flags & devFlags[i].dwValue) {
                    lpType = devFlags[i].lpDescription;
                    subitems.Count = 2;

                    if (j == 0) {
                        //add first entry with flag description
                        szValue1[0] = L'0';
                        szValue1[1] = L'x';
                        ultohex(devObject.Flags, &szValue1[2]);

                        subitems.Text[0] = szValue1;
                        subitems.Text[1] = lpType;
                    }
                    else {
                        //add subentry
                        subitems.Text[0] = NULL;
                        subitems.Text[1] = lpType;
                    }

                    TreeListAddItem(g_TreeList, h_tviRootItem, TVIF_TEXT | TVIF_STATE, 0,
                        TVIS_EXPANDED, (j == 0) ? T_FLAGS : NULL, &subitems);

                    devObject.Flags &= ~devFlags[i].dwValue;
                    j++;
                }
                if (devObject.Flags == 0) {
                    break;
                }
            }
        }
        else {
            //add named entry with zero data
            ObDumpUlong(g_TreeList, h_tviRootItem, T_FLAGS, NULL, 0, TRUE, FALSE, 0, 0);
        }

        //Characteristics
        RtlSecureZeroMemory(&szValue1, sizeof(szValue1));
        RtlSecureZeroMemory(&subitems, sizeof(subitems));

        lpType = NULL;
        j = 0;
        if (devObject.Characteristics) {
            for (i = 0; i < MAX_KNOWN_CHR_FLAGS; i++) {

                if (devObject.Characteristics & devChars[i].dwValue) {
                    lpType = devChars[i].lpDescription;
                    subitems.Count = 2;

                    if (j == 0) {
                        //add first entry with chr description
                        szValue1[0] = L'0';
                        szValue1[1] = L'x';
                        ultohex(devObject.Characteristics, &szValue1[2]);
                        subitems.Text[0] = szValue1;
                        subitems.Text[1] = lpType;
                    }
                    else {
                        //add subentry
                        subitems.Text[0] = NULL;
                        subitems.Text[1] = lpType;
                    }

                    TreeListAddItem(g_TreeList, h_tviRootItem, TVIF_TEXT | TVIF_STATE, 0,
                        0, (j == 0) ? T_CHARACTERISTICS : NULL, &subitems);

                    devObject.Characteristics &= ~devChars[i].dwValue;
                    j++;
                }

                if (devObject.Characteristics == 0) {
                    break;
                }
            }
        }
        else {
            //add zero value
            ObDumpUlong(g_TreeList, h_tviRootItem, T_CHARACTERISTICS, NULL, 0, TRUE, FALSE, 0, 0);
        }

        //Vpb
        lpType = L"PVPB";
        ObDumpAddress(g_TreeList, h_tviRootItem, L"Vpb", lpType, devObject.Vpb, 0, 0);

        //DeviceExtension
        ObDumpAddress(g_TreeList, h_tviRootItem, L"DeviceExtension", NULL, devObject.DeviceExtension, 0, 0);

        //DeviceType
        lpType = NULL;
        for (i = 0; i < MAX_DEVOBJ_CHARS; i++) {
            if (devObjChars[i].dwValue == devObject.DeviceType) {
                lpType = devObjChars[i].lpDescription;
                break;
            }
        }
        ObDumpUlong(g_TreeList, h_tviRootItem, L"DeviceType", lpType, devObject.DeviceType, TRUE, FALSE, 0, 0);

        //StackSize
        ObDumpUlong(g_TreeList, h_tviRootItem, L"StackSize", NULL, devObject.StackSize, FALSE, FALSE, 0, 0);

        //Queue
        h_tviSubItem = TreeListAddItem(g_TreeList, h_tviRootItem, TVIF_TEXT | TVIF_STATE, 0,
            TVIS_EXPANDED, L"Queue", NULL);

        //Queue->Wcb
        h_tviWcb = TreeListAddItem(g_TreeList, h_tviSubItem, TVIF_TEXT | TVIF_STATE, 0,
            TVIS_EXPANDED, L"Wcb", NULL);

        //Queue->Wcb->WaitQueueEntry
        h_tviWaitEntry = TreeListAddItem(g_TreeList, h_tviWcb, TVIF_TEXT | TVIF_STATE, 0,
            TVIS_EXPANDED, L"WaitQueueEntry", NULL);

        //Queue->Wcb->WaitQueueEntry->DeviceListEntry
        ObDumpListEntry(g_TreeList, h_tviWaitEntry, L"DeviceListEntry", &devObject.Queue.Wcb.WaitQueueEntry.DeviceListEntry);

        //Queue->Wcb->WaitQueueEntry->SortKey
        ObDumpUlong(g_TreeList, h_tviWaitEntry, L"SortKey", NULL, devObject.Queue.Wcb.WaitQueueEntry.SortKey, TRUE, FALSE, 0, 0);

        //Queue->Wcb->WaitQueueEntry->Inserted
        ObDumpByte(g_TreeList, h_tviWaitEntry, L"Inserted", NULL, devObject.Queue.Wcb.WaitQueueEntry.Inserted, 0, 0, TRUE);

        //Queue->Wcb->DmaWaitEntry
        ObDumpListEntry(g_TreeList, h_tviWcb, L"DmaWaitEntry", &devObject.Queue.Wcb.DmaWaitEntry);

        //Queue->Wcb->NumberOfChannels
        ObDumpUlong(g_TreeList, h_tviWcb, L"NumberOfChannels", NULL, devObject.Queue.Wcb.NumberOfChannels, FALSE, FALSE, 0, 0);

        //Queue->Wcb->SyncCallback
        ObDumpUlong(g_TreeList, h_tviWcb, L"SyncCallback", NULL, devObject.Queue.Wcb.SyncCallback, FALSE, FALSE, 0, 0);

        //Queue->Wcb->DmaContext
        ObDumpUlong(g_TreeList, h_tviWcb, L"DmaContext", NULL, devObject.Queue.Wcb.DmaContext, FALSE, FALSE, 0, 0);

        //Queue->Wcb->DeviceRoutine
        lpType = L"PDRIVER_CONTROL";
        ObDumpAddress(g_TreeList, h_tviWcb, L"DeviceRoutine", lpType, devObject.Queue.Wcb.DeviceRoutine, 0, 0);

        //Queue->Wcb->DeviceContext
        ObDumpAddress(g_TreeList, h_tviWcb, L"DeviceContext", NULL, devObject.Queue.Wcb.DeviceContext, 0, 0);

        //Queue->Wcb->NumberOfMapRegisters
        ObDumpUlong(g_TreeList, h_tviWcb, L"DeviceContext", NULL, devObject.Queue.Wcb.NumberOfMapRegisters, FALSE, FALSE, 0, 0);

        //Queue->Wcb->DeviceObject
        lpType = NULL;
        BgColor = 0;
        if (devObject.Queue.Wcb.DeviceObject != NULL) {

            LookupObject = ObCollectionFindByAddress(
                &g_kdctx.ObCollection,
                (ULONG_PTR)devObject.Queue.Wcb.DeviceObject,
                FALSE);

            if (LookupObject != NULL) {
                lpType = LookupObject->ObjectName;
            }
            else {
                lpType = L"Unnamed";
                BgColor = CLR_LGRY;
            }
        }
        ObDumpAddress(g_TreeList, h_tviWcb, L"DeviceObject", lpType, devObject.Queue.Wcb.DeviceObject, BgColor, 0);

        //Queue->Wcb->CurrentIrp
        ObDumpAddress(g_TreeList, h_tviWcb, L"CurrentIrp", NULL, devObject.Queue.Wcb.CurrentIrp, 0, 0);

        //Queue->Wcb->BufferChainingDpc
        lpType = T_PKDPC;
        ObDumpAddress(g_TreeList, h_tviWcb, L"BufferChainingDpc", lpType, devObject.Queue.Wcb.BufferChainingDpc, 0, 0);

        //AlignmentRequirement
        lpType = NULL;
        for (i = 0; i < MAX_KNOWN_FILEALIGN; i++) {
            if (fileAlign[i].dwValue == devObject.AlignmentRequirement) {
                lpType = fileAlign[i].lpDescription;
                break;
            }
        }
        ObDumpUlong(g_TreeList, h_tviRootItem, L"AlignmentRequirement", lpType, devObject.AlignmentRequirement, TRUE, FALSE, 0, 0);

        //DeviceQueue
        h_tviSubItem = TreeListAddItem(g_TreeList, h_tviRootItem, TVIF_TEXT | TVIF_STATE, 0,
            TVIS_EXPANDED, L"DeviceQueue", NULL);

        //DeviceQueue->Type
        lpType = L"KOBJECTS";
        ObDumpUlong(g_TreeList, h_tviSubItem, L"Type", lpType, devObject.DeviceQueue.Type, TRUE, TRUE, 0, 0);

        //DeviceQueue->Size
        ObDumpUlong(g_TreeList, h_tviSubItem, L"Size", NULL, devObject.DeviceQueue.Size, TRUE, TRUE, 0, 0);

        //DeviceQueue->DeviceListHead
        ObDumpListEntry(g_TreeList, h_tviSubItem, L"DeviceListHead", &devObject.DeviceQueue.DeviceListHead);

        //DeviceQueue->Lock
        ObDumpAddress(g_TreeList, h_tviSubItem, L"Lock", NULL, (PVOID)devObject.DeviceQueue.Lock, 0, 0);

        //DeviceQueue->Busy
        ObDumpByte(g_TreeList, h_tviSubItem, L"Busy", NULL, devObject.DeviceQueue.Busy, 0, 0, TRUE);

        //DeviceQueue->Hint
        ObDumpAddress(g_TreeList, h_tviSubItem, L"Hint", NULL, (PVOID)devObject.DeviceQueue.Hint, 0, 0);

        //
        //DEVICE_OBJECT->Dpc
        //
        h_tviSubItem = TreeListAddItem(g_TreeList, h_tviRootItem, TVIF_TEXT | TVIF_STATE, 0,
            TVIS_EXPANDED, L"Dpc", NULL);

        lpType = NULL;
        if (devObject.Dpc.Type == DPC_NORMAL) lpType = L"DPC_NORMAL";
        if (devObject.Dpc.Type == DPC_THREADED) lpType = L"DPC_THREADED";
        ObDumpUlong(g_TreeList, h_tviSubItem, L"Type", lpType, devObject.Dpc.Type, TRUE, TRUE, 0, 0);
        lpType = NULL;
        if (devObject.Dpc.Importance == LowImportance) lpType = L"LowImportance";
        if (devObject.Dpc.Importance == MediumImportance) lpType = L"MediumImportance";
        if (devObject.Dpc.Importance == HighImportance) lpType = L"HighImportance";
        ObDumpUlong(g_TreeList, h_tviSubItem, L"Importance", lpType, devObject.Dpc.Importance, TRUE, TRUE, 0, 0);
        ObDumpUlong(g_TreeList, h_tviSubItem, L"Number", NULL, devObject.Dpc.Number, TRUE, TRUE, 0, 0);

        //Dpc->DpcListEntry
        ObDumpAddress(g_TreeList, h_tviSubItem, L"DpcListEntry", NULL, (PVOID)devObject.Dpc.DpcListEntry.Next, 0, 0);

        //Dpc->ProcessorHistory
        ObDumpAddress(g_TreeList, h_tviSubItem, L"ProcessorHistory", NULL, (PVOID)devObject.Dpc.ProcessorHistory, 0, 0);

        //Dpc->DeferredRoutine
        ObDumpAddress(g_TreeList, h_tviSubItem, L"DeferredRoutine", NULL, devObject.Dpc.DeferredRoutine, 0, 0);

        //Dpc->DeferredContext
        ObDumpAddress(g_TreeList, h_tviSubItem, L"DeferredContext", NULL, devObject.Dpc.DeferredContext, 0, 0);

        //Dpc->SystemArgument1
        ObDumpAddress(g_TreeList, h_tviSubItem, L"SystemArgument1", NULL, devObject.Dpc.SystemArgument1, 0, 0);

        //Dpc->SystemArgument2
        ObDumpAddress(g_TreeList, h_tviSubItem, L"SystemArgument2", NULL, devObject.Dpc.SystemArgument2, 0, 0);

        //ActiveThreadCount
        ObDumpUlong(g_TreeList, h_tviRootItem, L"ActiveThreadCount", NULL, devObject.ActiveThreadCount, FALSE, FALSE, 0, 0);

        //SecurityDescriptor
        lpType = L"PSECURITY_DESCRIPTOR";
        ObDumpAddress(g_TreeList, h_tviRootItem, L"SecurityDescriptor", lpType, devObject.SecurityDescriptor, 0, 0);

        //DeviceLock
        h_tviWaitEntry = TreeListAddItem(g_TreeList, h_tviRootItem, TVIF_TEXT | TVIF_STATE, 0,
            TVIS_EXPANDED, L"DeviceLock", NULL);

        //DeviceLock->Header
        ObDumpDispatcherHeader(h_tviWaitEntry, &devObject.DeviceLock.Header, NULL, NULL, NULL);

        //SectorSize
        ObDumpUlong(g_TreeList, h_tviRootItem, L"SectorSize", NULL, devObject.SectorSize, TRUE, TRUE, 0, 0);
        //Spare
        ObDumpUlong(g_TreeList, h_tviRootItem, L"Spare1", NULL, devObject.Spare1, TRUE, TRUE, 0, 0);

        //DeviceObjectExtension
        lpType = L"PDEVOBJ_EXTENSION";
        ObDumpAddress(g_TreeList, h_tviRootItem, L"DeviceObjectExtension", lpType, devObject.DeviceObjectExtension, 0, 0);

        //Reserved
        ObDumpAddress(g_TreeList, h_tviRootItem, L"Reserved", NULL, devObject.Reserved, 0, 0);

        //
        //DEVOBJ_EXTENSION
        //

        if (devObject.DeviceObjectExtension) {

            RtlSecureZeroMemory(&devObjExt, sizeof(devObjExt));

            if (!kdReadSystemMemoryEx(
                (ULONG_PTR)devObject.DeviceObjectExtension,
                &devObjExt,
                sizeof(devObjExt),
                NULL))
            {
                return; //safe to exit, nothing after this
            }

            h_tviRootItem = TreeListAddItem(g_TreeList, NULL, TVIF_TEXT | TVIF_STATE, 0,
                TVIS_EXPANDED, L"DEVOBJ_EXTENSION", NULL);

            BgColor = 0;
            lpType = L"IO_TYPE_DEVICE_OBJECT_EXTENSION";
            if (devObjExt.Type != IO_TYPE_DEVICE_OBJECT_EXTENSION) {
                lpType = L"! Must be IO_TYPE_DEVICE_OBJECT_EXTENSION";
                BgColor = CLR_WARN;
            }
            //Type
            ObDumpUlong(g_TreeList, h_tviRootItem, L"Type", lpType, devObjExt.Type, TRUE, TRUE, BgColor, 0);
            //Size
            ObDumpUlong(g_TreeList, h_tviRootItem, L"Size", NULL, devObjExt.Size, TRUE, TRUE, 0, 0);

            //DeviceObject
            lpType = NULL;
            BgColor = 0;
            if (devObjExt.DeviceObject != NULL) {

                LookupObject = ObCollectionFindByAddress(
                    &g_kdctx.ObCollection,
                    (ULONG_PTR)devObjExt.DeviceObject,
                    FALSE);

                if (LookupObject != NULL) {
                    lpType = LookupObject->ObjectName;
                }
                else {
                    lpType = L"Unnamed";
                    BgColor = CLR_LGRY;
                }
            }
            ObDumpAddress(g_TreeList, h_tviRootItem, L"DeviceObject", lpType, devObjExt.DeviceObject, BgColor, 0);

            //PowerFlags
            ObDumpUlong(g_TreeList, h_tviRootItem, L"PowerFlags", NULL, devObjExt.PowerFlags, TRUE, FALSE, 0, 0);

            //Dope
            lpType = L"PDEVICE_OBJECT_POWER_EXTENSION";
            ObDumpAddress(g_TreeList, h_tviRootItem, L"Dope", lpType, devObjExt.Dope, 0, 0);

            //ExtensionFlags
            ObDumpUlong(g_TreeList, h_tviRootItem, L"ExtensionFlags", NULL, devObjExt.ExtensionFlags, TRUE, FALSE, 0, 0);

            //DeviceNode
            lpType = L"PDEVICE_NODE";
            ObDumpAddress(g_TreeList, h_tviRootItem, L"DeviceNode", lpType, devObjExt.DeviceNode, 0, 0);

            //AttachedTo
            lpType = NULL;
            BgColor = 0;
            if (devObjExt.AttachedTo != NULL) {

                LookupObject = ObCollectionFindByAddress(
                    &g_kdctx.ObCollection,
                    (ULONG_PTR)devObjExt.AttachedTo,
                    FALSE);

                if (LookupObject != NULL) {
                    lpType = LookupObject->ObjectName;
                }
                else {
                    lpType = T_UNNAMED;
                    BgColor = CLR_LGRY;
                }
            }
            ObDumpAddress(g_TreeList, h_tviRootItem, L"AttachedTo", lpType, devObjExt.AttachedTo, BgColor, 0);
        }
    }
    __except (exceptFilter(GetExceptionCode(), GetExceptionInformation())) {
        return;
    }
}

/*
* ObxDumpSessionIdVersionAware
*
* Purpose:
*
* Dump OBJECT_DIRECTORY SessionId.
*
*/
VOID ObxDumpSessionIdVersionAware(
    HTREEITEM h_tviRootItem,
    _In_ ULONG SessionId
)
{
    LPWSTR lpType;

    if (SessionId == OBJ_INVALID_SESSION_ID)
        lpType = T_OBJ_INVALID_SESSION_ID;
    else
        lpType = NULL;

    ObDumpUlong(g_TreeList, h_tviRootItem, TEXT("SessionId"), lpType, SessionId, TRUE, FALSE, 0, 0);
}

/*
* ObDumpDirectoryObject
*
* Purpose:
*
* Dump OBJECT_DIRECTORY members to the treelist.
*
*/
VOID ObDumpDirectoryObject(
    _In_ PROP_OBJECT_INFO *Context,
    _In_ HWND hwndDlg
)
{
    INT                     i;
    HTREEITEM               h_tviRootItem, h_tviSubItem, h_tviEntry;
    LPWSTR                  lpType;
    OBJECT_DIRECTORY        dirObject;
    OBJECT_DIRECTORY_V2     dirObjectV2;
    OBJECT_DIRECTORY_V3     dirObjectV3;
    OBJECT_DIRECTORY_ENTRY  dirEntry;
    LIST_ENTRY              ChainLink;
    TL_SUBITEMS_FIXED       subitems;
    WCHAR                   szId[MAX_PATH + 1], szValue[MAX_PATH + 1];

    ULONG ObjectVersion;
    ULONG ObjectSize;
    PVOID ObjectPtr;
    OBJECT_DIRECTORY_V3 *pCompatDirObject;

    if (Context == NULL) {
        return;
    }

    __try {

        switch (g_NtBuildNumber) {

        case 7600:
        case 7601:
        case 9200:
        case 9600:
            ObjectVersion = 1;
            ObjectSize = sizeof(OBJECT_DIRECTORY);
            ObjectPtr = &dirObject;
            break;

        case 10240:
        case 10586:
        case 14393:
            ObjectVersion = 2;
            ObjectSize = sizeof(OBJECT_DIRECTORY_V2);
            ObjectPtr = &dirObjectV2;
            break;

        default:
            ObjectVersion = 3;
            ObjectSize = sizeof(OBJECT_DIRECTORY_V3);
            ObjectPtr = &dirObjectV3;
            break;
        }

        RtlSecureZeroMemory(&dirObject, sizeof(dirObject));
        RtlSecureZeroMemory(&dirObjectV2, sizeof(dirObjectV2));
        RtlSecureZeroMemory(&dirObjectV3, sizeof(dirObjectV3));
        pCompatDirObject = &dirObjectV3;

        //
        // Dump DIRECTORY_OBJECT.
        //
        // Handle different object versions.
        //
        if (!kdReadSystemMemoryEx(
            Context->ObjectInfo.ObjectAddress,
            ObjectPtr,
            ObjectSize,
            NULL))
        {
            ObDumpShowError(hwndDlg);
            return;
        }

        //
        // Build compatible object to work with based on newest OBJECT_DIRECTORY variant (since it has all fields).
        //
        switch (ObjectVersion) {
        case 1:

            RtlCopyMemory(
                pCompatDirObject->HashBuckets,
                &dirObject.HashBuckets,
                sizeof(dirObject.HashBuckets));

            pCompatDirObject->Lock = dirObject.Lock;
            pCompatDirObject->Flags = dirObject.Flags;
            pCompatDirObject->NamespaceEntry = dirObject.NamespaceEntry;
            pCompatDirObject->SessionId = dirObject.SessionId;
            pCompatDirObject->DeviceMap = dirObject.DeviceMap;
            pCompatDirObject->ShadowDirectory = NULL; //union with DeviceMap in 8.1 no sense to output differently

            break;

        case 2:
            RtlCopyMemory(
                pCompatDirObject->HashBuckets,
                &dirObjectV2.HashBuckets,
                sizeof(dirObjectV2.HashBuckets));

            pCompatDirObject->Lock = dirObjectV2.Lock;
            pCompatDirObject->Flags = dirObjectV2.Flags;
            pCompatDirObject->NamespaceEntry = dirObjectV2.NamespaceEntry;
            pCompatDirObject->SessionId = dirObjectV2.SessionId;
            pCompatDirObject->DeviceMap = dirObjectV2.DeviceMap;
            pCompatDirObject->ShadowDirectory = dirObjectV2.ShadowDirectory;
            pCompatDirObject->SessionObject = NULL;
            break;

        case 3:
        default:

            //
            // Do nothing, everything read already.
            //
            break;
        }


        g_TreeList = 0;
        g_TreeListAtom = 0;
        if (!supInitTreeListForDump(hwndDlg, &g_TreeListAtom, &g_TreeList)) {
            ObDumpShowError(hwndDlg);
            return;
        }

        //
        //OBJECT_DIRECTORY
        //
        h_tviRootItem = TreeListAddItem(
            g_TreeList,
            NULL,
            TVIF_TEXT | TVIF_STATE,
            TVIS_EXPANDED,
            TVIS_EXPANDED,
            T_OBJECT_DIRECTORY,
            NULL);

        RtlSecureZeroMemory(&subitems, sizeof(subitems));
        subitems.Count = 1;
        subitems.Text[0] = TEXT("{...}");

        h_tviSubItem = TreeListAddItem(
            g_TreeList,
            h_tviRootItem,
            TVIF_TEXT | TVIF_STATE,
            0,
            0,
            TEXT("HashBuckets"),
            &subitems);

        for (i = 0; i < NUMBER_HASH_BUCKETS; i++) {
            RtlSecureZeroMemory(&subitems, sizeof(subitems));
            subitems.Count = 2;

            RtlSecureZeroMemory(szId, sizeof(szId));
            wsprintf(szId, TEXT("[ %i ]"), i);

            if (pCompatDirObject->HashBuckets[i]) {
                RtlSecureZeroMemory(szValue, sizeof(szValue));
                szValue[0] = L'0';
                szValue[1] = L'x';
                u64tohex((ULONG_PTR)pCompatDirObject->HashBuckets[i], &szValue[2]);
                subitems.Text[0] = szValue;
                subitems.Text[1] = T_POBJECT_DIRECTORY_ENTRY;
            }
            else {
                subitems.Text[0] = T_NULL;
            }

            h_tviEntry = TreeListAddItem(
                g_TreeList,
                h_tviSubItem,
                TVIF_TEXT | TVIF_STATE,
                0,
                0,
                szId,
                &subitems);

            //dump entry if available
            if (pCompatDirObject->HashBuckets[i]) {

                RtlSecureZeroMemory(&dirEntry, sizeof(dirEntry));

                if (kdReadSystemMemoryEx(
                    (ULONG_PTR)pCompatDirObject->HashBuckets[i],
                    &dirEntry,
                    sizeof(dirEntry),
                    NULL))
                {

                    ChainLink.Blink = NULL;
                    ChainLink.Flink = NULL;
                    lpType = TEXT("ChainLink");
                    if (dirEntry.ChainLink == NULL) {
                        ObDumpAddress(g_TreeList, h_tviEntry, lpType, T_PLIST_ENTRY, NULL, 0, 0);
                    }
                    else {
                        if (kdReadSystemMemoryEx(
                            (ULONG_PTR)dirEntry.ChainLink,
                            &ChainLink,
                            sizeof(ChainLink),
                            NULL))
                        {
                            ObDumpListEntry(g_TreeList, h_tviEntry, lpType, &ChainLink);
                        }
                        else {
                            ObDumpAddress(g_TreeList, h_tviEntry, lpType, T_PLIST_ENTRY, dirEntry.ChainLink, 0, 0);
                        }
                    }
                    ObDumpAddress(g_TreeList, h_tviEntry, TEXT("Object"), NULL, dirEntry.Object, 0, 0);
                    ObDumpUlong(g_TreeList, h_tviEntry, TEXT("HashValue"), NULL, dirEntry.HashValue, TRUE, FALSE, 0, 0);
                }
            }
        }

        //EX_PUSH_LOCK
        RtlSecureZeroMemory(&subitems, sizeof(subitems));
        subitems.Count = 2;
        subitems.Text[1] = T_EX_PUSH_LOCK;

        h_tviSubItem = TreeListAddItem(
            g_TreeList,
            h_tviRootItem,
            TVIF_TEXT | TVIF_STATE,
            0,
            0,
            TEXT("Lock"),
            &subitems);

        ObDumpAddress(g_TreeList, h_tviSubItem, TEXT("Ptr"), NULL, pCompatDirObject->Lock.Ptr, 0, 0);

        //DeviceMap
        ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("DeviceMap"), T_PDEVICE_MAP, pCompatDirObject->DeviceMap, 0, 0);

        //ShadowDirectory
        if (ObjectVersion != 1) {
            ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("ShadowDirectory"), T_POBJECT_DIRECTORY, pCompatDirObject->ShadowDirectory, 0, 0);
        }

        //
        // Handle different object versions fields order.
        //

        //
        // SessionId
        //
        if (ObjectVersion != 3) {

            ObxDumpSessionIdVersionAware(
                h_tviRootItem,
                pCompatDirObject->SessionId);

        }

        ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("NamespaceEntry"), NULL, pCompatDirObject->NamespaceEntry, 0, 0);

        if (ObjectVersion == 3) {
            ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("SessionObject"), NULL, pCompatDirObject->SessionObject, 0, 0);
        }

        ObDumpUlong(g_TreeList, h_tviRootItem, TEXT("Flags"), NULL, pCompatDirObject->Flags, TRUE, FALSE, 0, 0);

        //
        // SessionId is the last member of OBJECT_DIRECTORY_V3
        //
        if (ObjectVersion == 3) {

            ObxDumpSessionIdVersionAware(
                h_tviRootItem,
                pCompatDirObject->SessionId);
        }

    }
    __except (exceptFilter(GetExceptionCode(), GetExceptionInformation())) {
        return;
    }
}

/*
* ObDumpSyncObject
*
* Purpose:
*
* Dump KEVENT/KMUTANT/KSEMAPHORE/KTIMER members to the treelist.
*
*/
VOID ObDumpSyncObject(
    _In_ PROP_OBJECT_INFO *Context,
    _In_ HWND hwndDlg
)
{
    HTREEITEM            h_tviRootItem;
    LPWSTR               lpType = NULL, lpDescType = NULL, lpDesc1 = NULL, lpDesc2 = NULL;
    KMUTANT             *Mutant = NULL;
    KEVENT              *Event = NULL;
    KSEMAPHORE          *Semaphore = NULL;
    KTIMER              *Timer = NULL;
    DISPATCHER_HEADER   *Header = NULL;
    PVOID                Object = NULL;
    ULONG                ObjectSize = 0UL;
    WCHAR                szValue[MAX_PATH + 1];

    if (Context == NULL) {
        return;
    }

    __try {

        switch (Context->TypeIndex) {

        case ObjectTypeEvent:
            ObjectSize = sizeof(KEVENT);
            break;

        case ObjectTypeMutant:
            ObjectSize = sizeof(KMUTANT);
            break;

        case ObjectTypeSemaphore:
            ObjectSize = sizeof(KSEMAPHORE);
            break;

        case ObjectTypeTimer:
            ObjectSize = sizeof(KTIMER);
            break;

        }

        Object = supHeapAlloc(ObjectSize);
        if (Object == NULL) {
            ObDumpShowError(hwndDlg);
            return;
        }

        //dump object
        if (!kdReadSystemMemoryEx(
            Context->ObjectInfo.ObjectAddress,
            Object,
            ObjectSize,
            NULL))
        {
            ObDumpShowError(hwndDlg);
            supHeapFree(Object);
            return;
        }

        g_TreeList = 0;
        g_TreeListAtom = 0;
        if (!supInitTreeListForDump(hwndDlg, &g_TreeListAtom, &g_TreeList)) {
            ObDumpShowError(hwndDlg);
            supHeapFree(Object);
            return;
        }

        //
        //Object name
        //
        Header = NULL;
        switch (Context->TypeIndex) {
        case ObjectTypeEvent:
            lpType = T_KEVENT;
            Event = (KEVENT*)Object;
            Header = &Event->Header;

            lpDescType = T_UnknownType;
            switch (Header->Type) {
            case NotificationEvent:
                lpDescType = T_EVENT_NOTIFICATION;
                break;
            case SynchronizationEvent:
                lpDescType = T_EVENT_SYNC;
                break;
            }

            //Event state
            lpDesc1 = T_Unknown;
            switch (Header->SignalState) {
            case 0:
                lpDesc1 = T_NONSIGNALED;
                break;
            case 1:
                lpDesc1 = T_SIGNALED;
                break;
            }

            lpDesc2 = NULL;
            if (Header->Size == (sizeof(KEVENT) / sizeof(ULONG))) {
                lpDesc2 = L"sizeof(KEVENT)/sizeof(ULONG)";
            }
            break;

        case ObjectTypeMutant:
            lpType = T_KMUTANT;
            Mutant = (KMUTANT*)Object;
            Header = &Mutant->Header;
            lpDesc1 = L"Not Held";
            RtlSecureZeroMemory(szValue, sizeof(szValue));
            if (Mutant->OwnerThread != NULL) {
                wsprintf(szValue, L"Held %d times", Header->SignalState);
                lpDesc1 = szValue;
            }

            lpDesc2 = NULL;
            if (Header->Size == (sizeof(KMUTANT) / sizeof(ULONG))) {
                lpDesc2 = L"sizeof(KMUTANT)/sizeof(ULONG)";
            }
            break;

        case ObjectTypeSemaphore:
            lpType = T_KSEMAPHORE;
            Semaphore = (KSEMAPHORE*)Object;
            Header = &Semaphore->Header;

            lpDesc1 = L"Count";
            lpDesc2 = NULL;
            if (Header->Size == (sizeof(KSEMAPHORE) / sizeof(ULONG))) {
                lpDesc2 = L"sizeof(KSEMAPHORE)/sizeof(ULONG)";
            }
            break;

        case ObjectTypeTimer:
            lpType = T_KTIMER;
            Timer = (KTIMER*)Object;
            Header = &Timer->Header;

            lpDescType = T_TIMER_SYNC;
            if (Header->TimerType == 8) {
                lpDescType = T_TIMER_NOTIFICATION;
            }
            //Timer state
            lpDesc1 = T_Unknown;
            switch (Header->SignalState) {
            case 0:
                lpDesc1 = T_NONSIGNALED;
                break;
            case 1:
                lpDesc1 = T_SIGNALED;
                break;
            }
            lpDesc2 = NULL;
            break;

        }

        if (Header == NULL) {
            ObDumpShowError(hwndDlg);
            supHeapFree(Object);
            return;
        }

        h_tviRootItem = TreeListAddItem(
            g_TreeList,
            NULL,
            TVIF_TEXT | TVIF_STATE,
            TVIS_EXPANDED,
            TVIS_EXPANDED,
            lpType,
            NULL);

        //Header
        ObDumpDispatcherHeader(h_tviRootItem, Header, lpDescType, lpDesc1, lpDesc2);

        //type specific values
        switch (Context->TypeIndex) {
        case ObjectTypeMutant:
            if (Mutant) {
                ObDumpListEntry(g_TreeList, h_tviRootItem, L"MutantListEntry", &Mutant->MutantListEntry);
                ObDumpAddress(g_TreeList, h_tviRootItem, L"OwnerThread", T_PKTHREAD, Mutant->OwnerThread, 0, 0);
                ObDumpByte(g_TreeList, h_tviRootItem, L"Abandoned", NULL, Mutant->Abandoned, 0, 0, TRUE);
                ObDumpByte(g_TreeList, h_tviRootItem, L"ApcDisable", NULL, Mutant->ApcDisable, 0, 0, FALSE);
            }
            break;

        case ObjectTypeSemaphore:
            if (Semaphore) {
                ObDumpUlong(g_TreeList, h_tviRootItem, L"Limit", NULL, Semaphore->Limit, TRUE, FALSE, 0, 0);
            }
            break;

        case ObjectTypeTimer:
            if (Timer) {
                ObDumpULargeInteger(g_TreeList, h_tviRootItem, L"DueTime", &Timer->DueTime); //dumped as hex, not important
                ObDumpListEntry(g_TreeList, h_tviRootItem, L"TimerListEntry", &Timer->TimerListEntry);
                ObDumpAddress(g_TreeList, h_tviRootItem, L"Dpc", T_PKDPC, Timer->Dpc, 0, 0);
                ObDumpUlong(g_TreeList, h_tviRootItem, L"Processor", NULL, Timer->Processor, TRUE, FALSE, 0, 0);
                ObDumpUlong(g_TreeList, h_tviRootItem, L"Period", NULL, Timer->Period, TRUE, FALSE, 0, 0);
            }
            break;

        }

        supHeapFree(Object);
    }
    __except (exceptFilter(GetExceptionCode(), GetExceptionInformation())) {
        return;
    }
}

/*
* ObDumpObjectTypeFlags
*
* Purpose:
*
* Dump ObjectTypeFlags/ObjectTypeFlags2 bits to the treelist.
*
*/
VOID ObDumpObjectTypeFlags(
    _In_ LPWSTR EntryName,
    _In_ UCHAR ObjectTypeFlags,
    _In_ HTREEITEM h_tviSubItem,
    _In_ LPWSTR *ObjectTypeFlagsText,
    _In_ BOOLEAN SetEntry
)
{
    ULONG i, j;
    LPWSTR lpType;
    TL_SUBITEMS_FIXED TreeListSubitems;

    WCHAR szValue[100];

    if (ObjectTypeFlags) {

        RtlSecureZeroMemory(&TreeListSubitems, sizeof(TreeListSubitems));
        TreeListSubitems.Count = 2;

        j = 0;
        for (i = 0; i < 8; i++) {
            if (GET_BIT(ObjectTypeFlags, i)) {
                lpType = (LPWSTR)ObjectTypeFlagsText[i];
                TreeListSubitems.Text[0] = NULL;
                if (j == 0) {
                    wsprintf(szValue, FORMAT_HEXBYTE, ObjectTypeFlags);
                    TreeListSubitems.Text[0] = szValue;
                }
                TreeListSubitems.Text[1] = lpType;
                TreeListAddItem(g_TreeList, h_tviSubItem, TVIF_TEXT | TVIF_STATE, 0,
                    0, (j == 0) ? ((SetEntry) ? EntryName : NULL) : NULL, &TreeListSubitems);
                j++;
            }
        }
    }
    else {
        if (SetEntry)
            ObDumpByte(g_TreeList, h_tviSubItem, EntryName, NULL, ObjectTypeFlags, 0, 0, FALSE);
    }
}

/*
* ObDumpObjectType
*
* Purpose:
*
* Dump OBJECT_TYPE members to the treelist.
*
*/
VOID ObDumpObjectType(
    _In_ PROP_OBJECT_INFO *Context,
    _In_ HWND hwndDlg
)
{
    BOOL                    bCond = FALSE, bOkay;
    HTREEITEM               h_tviRootItem, h_tviSubItem, h_tviGenericMapping;
    UINT                    i;
    LPWSTR                  lpType = NULL;
    POBJINFO                CurrentObject = NULL;
    PVOID                   ObjectTypeInformation = NULL;
    PRTL_PROCESS_MODULES    ModulesList = NULL;
    TL_SUBITEMS_FIXED       TreeListSubitems;
    PVOID                   TypeProcs[MAX_KNOWN_OBJECT_TYPE_PROCEDURES];
    PVOID                   SelfDriverBase;
    ULONG                   SelfDriverSize;

    ULONG ObjectSize = 0;
    ULONG ObjectVersion = 0;

    BOOLEAN bSetEntry;

    ULONG Key;
    PVOID LockPtr;
    PLIST_ENTRY pListEntry;
    ULONG WaitObjectFlagMask;
    USHORT WaitObjectFlagOffset;
    USHORT WaitObjectPointerOffset;

    union {
        union {
            OBJECT_TYPE_COMPATIBLE* ObjectTypeCompatible;
            OBJECT_TYPE_7 *ObjectType_7;
            OBJECT_TYPE_8 *ObjectType_8;
            OBJECT_TYPE_RS1 *ObjectType_RS1;
            OBJECT_TYPE_RS2 *ObjectType_RS2;
        } Versions;
        PVOID Ref;
    } ObjectType;

    if (Context == NULL) {
        return;
    }

    do {

        bOkay = FALSE;

        //
        // Get loaded modules list.
        //
        ModulesList = (PRTL_PROCESS_MODULES)supGetSystemInfo(SystemModuleInformation);
        if (ModulesList == NULL)
            break;

        //
        // Get the reference to the object.
        //
        CurrentObject = ObQueryObject(T_OBJECTTYPES, Context->lpObjectName);
        if (CurrentObject == NULL)
            break;

        //
        // Dump object information version aware.
        //
        ObjectTypeInformation = ObDumpObjectTypeVersionAware(
            CurrentObject->ObjectAddress,
            &ObjectSize,
            &ObjectVersion);

        if (ObjectTypeInformation == NULL)
            break;

        //
        // For listing common fields.
        //
        ObjectType.Ref = ObjectTypeInformation;

        //
        // Initialize treelist.
        //
        g_TreeList = 0;
        g_TreeListAtom = 0;
        if (!supInitTreeListForDump(hwndDlg, &g_TreeListAtom, &g_TreeList))
            break;

        //
        // Add treelist root item ("OBJECT_TYPE").
        //
        h_tviRootItem = TreeListAddItem(
            g_TreeList,
            NULL,
            TVIF_TEXT | TVIF_STATE,
            TVIS_EXPANDED,
            TVIS_EXPANDED,
            T_OBJECT_TYPE,
            NULL);

        //
        // This fields are structure version unaware.
        //
        ObDumpListEntry(g_TreeList, h_tviRootItem, TEXT("TypeList"),
            &ObjectType.Versions.ObjectTypeCompatible->TypeList);

        ObDumpUnicodeString(h_tviRootItem, TEXT("Name"),
            &ObjectType.Versions.ObjectTypeCompatible->Name, FALSE);

        ObDumpAddress(g_TreeList, h_tviRootItem, TEXT("DefaultObject"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->DefaultObject, 0, 0);

        ObDumpByte(g_TreeList, h_tviRootItem, T_TYPEINDEX, NULL,
            ObjectType.Versions.ObjectTypeCompatible->Index, 0, 0, FALSE);

        ObDumpUlong(g_TreeList, h_tviRootItem, TEXT("TotalNumberOfObjects"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->TotalNumberOfObjects, TRUE, FALSE, 0, 0);

        ObDumpUlong(g_TreeList, h_tviRootItem, TEXT("TotalNumberOfHandles"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->TotalNumberOfHandles, TRUE, FALSE, 0, 0);

        ObDumpUlong(g_TreeList, h_tviRootItem, TEXT("HighWaterNumberOfObjects"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->HighWaterNumberOfObjects, TRUE, FALSE, 0, 0);

        ObDumpUlong(g_TreeList, h_tviRootItem, TEXT("HighWaterNumberOfHandles"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->HighWaterNumberOfHandles, TRUE, FALSE, 0, 0);

        //
        // OBJECT_TYPE_INITIALIZER
        //
        RtlSecureZeroMemory(&TreeListSubitems, sizeof(TreeListSubitems));

        TreeListSubitems.Count = 2;
        TreeListSubitems.Text[1] = T_OBJECT_TYPE_INITIALIZER;
        h_tviSubItem = TreeListAddItem(g_TreeList, h_tviRootItem, TVIF_TEXT | TVIF_STATE, 0,
            0, TEXT("TypeInfo"), &TreeListSubitems);

        ObDumpUlong(g_TreeList, h_tviSubItem, T_LENGTH, NULL,
            ObjectType.Versions.ObjectTypeCompatible->TypeInfo.Length, TRUE, TRUE, 0, 0);

        //
        // Dump Object Type Flags / Extended Object Type Flags
        //
        ObDumpObjectTypeFlags(T_OBJECT_TYPE_FLAGS,
            ObjectType.Versions.ObjectTypeCompatible->TypeInfo.ObjectTypeFlags,
            h_tviSubItem,
            (LPWSTR*)T_ObjectTypeFlags,
            TRUE);

        if (ObjectVersion > 2) {

            if (ObjectVersion == 3) {
                bSetEntry = TRUE;
                lpType = T_OBJECT_TYPE_FLAGS2; //fu ms
            }
            else {
                bSetEntry = FALSE;
                lpType = T_OBJECT_TYPE_FLAGS;
            }

            ObDumpObjectTypeFlags(lpType,
                ObjectType.Versions.ObjectType_RS1->TypeInfo.ObjectTypeFlags2,
                h_tviSubItem,
                (LPWSTR*)T_ObjectTypeFlags2,
                bSetEntry);

        }

        //
        // Structure version independent fields.
        //
        ObDumpUlong(g_TreeList, h_tviSubItem, TEXT("ObjectTypeCode"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->TypeInfo.ObjectTypeCode, TRUE, FALSE, 0, 0);

        ObDumpUlong(g_TreeList, h_tviSubItem, TEXT("InvalidAttributes"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->TypeInfo.InvalidAttributes, TRUE, FALSE, 0, 0);

        RtlSecureZeroMemory(&TreeListSubitems, sizeof(TreeListSubitems));
        TreeListSubitems.Count = 2;
        TreeListSubitems.Text[1] = T_GENERIC_MAPPING;
        h_tviGenericMapping = TreeListAddItem(g_TreeList, h_tviSubItem, TVIF_TEXT | TVIF_STATE, 0,
            0, TEXT("GenericMapping"), &TreeListSubitems);

        ObDumpUlong(g_TreeList, h_tviGenericMapping, TEXT("GenericRead"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->TypeInfo.GenericMapping.GenericRead, TRUE, FALSE, 0, 0);

        ObDumpUlong(g_TreeList, h_tviGenericMapping, TEXT("GenericWrite"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->TypeInfo.GenericMapping.GenericWrite, TRUE, FALSE, 0, 0);

        ObDumpUlong(g_TreeList, h_tviGenericMapping, TEXT("GenericExecute"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->TypeInfo.GenericMapping.GenericExecute, TRUE, FALSE, 0, 0);

        ObDumpUlong(g_TreeList, h_tviGenericMapping, TEXT("GenericAll"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->TypeInfo.GenericMapping.GenericAll, TRUE, FALSE, 0, 0);

        ObDumpUlong(g_TreeList, h_tviSubItem, TEXT("ValidAccessMask"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->TypeInfo.ValidAccessMask, TRUE, FALSE, 0, 0);
        ObDumpUlong(g_TreeList, h_tviSubItem, TEXT("RetainAccess"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->TypeInfo.RetainAccess, TRUE, FALSE, 0, 0);

        //Pool Type
        lpType = T_Unknown;
        for (i = 0; i < MAX_KNOWN_POOL_TYPES; i++) {
            if (ObjectType.Versions.ObjectTypeCompatible->TypeInfo.PoolType == (POOL_TYPE)a_PoolTypes[i].dwValue) {
                lpType = a_PoolTypes[i].lpDescription;
                break;
            }
        }

        ObDumpUlong(g_TreeList, h_tviSubItem, TEXT("PoolType"), lpType,
            ObjectType.Versions.ObjectTypeCompatible->TypeInfo.PoolType, TRUE, FALSE, 0, 0);

        ObDumpUlong(g_TreeList, h_tviSubItem, TEXT("DefaultPagedPoolCharge"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->TypeInfo.DefaultPagedPoolCharge, TRUE, FALSE, 0, 0);

        ObDumpUlong(g_TreeList, h_tviSubItem, TEXT("DefaultNonPagedPoolCharge"), NULL,
            ObjectType.Versions.ObjectTypeCompatible->TypeInfo.DefaultNonPagedPoolCharge, TRUE, FALSE, 0, 0);

        //
        // List callback procedures.
        //
        // Copy type procedures to temp array, assume DumpProcedure always first.
        //
        RtlSecureZeroMemory(TypeProcs, sizeof(TypeProcs));

        supCopyMemory(
            &TypeProcs,
            sizeof(TypeProcs),
            &ObjectType.Versions.ObjectTypeCompatible->TypeInfo.DumpProcedure,
            sizeof(TypeProcs));

        //assume ntoskrnl first in list and list initialized
        SelfDriverBase = ModulesList->Modules[0].ImageBase;
        SelfDriverSize = ModulesList->Modules[0].ImageSize;

        for (i = 0; i < MAX_KNOWN_OBJECT_TYPE_PROCEDURES; i++) {
            if (TypeProcs[i]) {
                ObDumpAddressWithModule(h_tviSubItem, T_TYPEPROCEDURES[i], TypeProcs[i],
                    ModulesList, SelfDriverBase, SelfDriverSize);
            }
            else {
                ObDumpAddress(g_TreeList, h_tviSubItem, T_TYPEPROCEDURES[i], NULL, TypeProcs[i], 0, 0);
            }
        }

        if (ObjectVersion > 1) {

            switch (ObjectVersion) {
            case 2:
                WaitObjectFlagMask = ObjectType.Versions.ObjectType_8->TypeInfo.WaitObjectFlagMask;
                WaitObjectFlagOffset = ObjectType.Versions.ObjectType_8->TypeInfo.WaitObjectFlagOffset;
                WaitObjectPointerOffset = ObjectType.Versions.ObjectType_8->TypeInfo.WaitObjectPointerOffset;
                break;
            case 3:
                WaitObjectFlagMask = ObjectType.Versions.ObjectType_RS1->TypeInfo.WaitObjectFlagMask;
                WaitObjectFlagOffset = ObjectType.Versions.ObjectType_RS1->TypeInfo.WaitObjectFlagOffset;
                WaitObjectPointerOffset = ObjectType.Versions.ObjectType_RS1->TypeInfo.WaitObjectPointerOffset;
                break;
            default:
                WaitObjectFlagMask = ObjectType.Versions.ObjectType_RS2->TypeInfo.WaitObjectFlagMask;
                WaitObjectFlagOffset = ObjectType.Versions.ObjectType_RS2->TypeInfo.WaitObjectFlagOffset;
                WaitObjectPointerOffset = ObjectType.Versions.ObjectType_RS2->TypeInfo.WaitObjectPointerOffset;
                break;
            }

            ObDumpUlong(g_TreeList, h_tviSubItem, TEXT("WaitObjectFlagMask"), NULL, WaitObjectFlagMask, TRUE, FALSE, 0, 0);
            ObDumpUlong(g_TreeList, h_tviSubItem, TEXT("WaitObjectFlagOffset"), NULL, WaitObjectFlagOffset, TRUE, TRUE, 0, 0);
            ObDumpUlong(g_TreeList, h_tviSubItem, TEXT("WaitObjectPointerOffset"), NULL, WaitObjectPointerOffset, TRUE, TRUE, 0, 0);

        }

        //
        // Rest of OBJECT_TYPE
        //
        switch (ObjectVersion) {
        case 1: //7
            Key = ObjectType.Versions.ObjectType_7->Key;
            LockPtr = ObjectType.Versions.ObjectType_7->TypeLock.Ptr;
            pListEntry = &ObjectType.Versions.ObjectType_7->CallbackList;
            break;

        case 2: //8+
            Key = ObjectType.Versions.ObjectType_8->Key;
            LockPtr = ObjectType.Versions.ObjectType_8->TypeLock.Ptr;
            pListEntry = &ObjectType.Versions.ObjectType_8->CallbackList;
            break;

        case 3: //RS1
            Key = ObjectType.Versions.ObjectType_RS1->Key;
            LockPtr = ObjectType.Versions.ObjectType_RS1->TypeLock.Ptr;
            pListEntry = &ObjectType.Versions.ObjectType_RS1->CallbackList;
            break;

        default: //RS2+
            Key = ObjectType.Versions.ObjectType_RS2->Key;
            LockPtr = ObjectType.Versions.ObjectType_RS2->TypeLock.Ptr;
            pListEntry = &ObjectType.Versions.ObjectType_RS2->CallbackList;
            break;
        }

        ObDumpPushLock(g_TreeList, h_tviRootItem, LockPtr, 0, 0);
        ObDumpUlong(g_TreeList, h_tviRootItem, TEXT("Key"), NULL, Key, TRUE, FALSE, 0, 0);
        ObDumpListEntry(g_TreeList, h_tviRootItem, TEXT("CallbackList"), pListEntry);

        bOkay = TRUE;

    } while (bCond);

    //
    // Cleanup.
    //
    if (ModulesList) supHeapFree(ModulesList);
    if (ObjectTypeInformation) supVirtualFree(ObjectTypeInformation);
    if (CurrentObject) supHeapFree(CurrentObject);

    //
    // Show error message on failure.
    //
    if (bOkay == FALSE) {
        ObDumpShowError(hwndDlg);
        return;
    }
}

/*
* ObDumpQueueObject
*
* Purpose:
*
* Dump KQUEUE members to the treelist.
*
*/
VOID ObDumpQueueObject(
    _In_ PROP_OBJECT_INFO *Context,
    _In_ HWND hwndDlg
)
{
    HTREEITEM h_tviRootItem;
    LPWSTR    lpDesc2;
    KQUEUE    Queue;

    if (Context == NULL) {
        return;
    }

    __try {

        //dump Queue object
        RtlSecureZeroMemory(&Queue, sizeof(Queue));

        if (!kdReadSystemMemoryEx(
            Context->ObjectInfo.ObjectAddress,
            &Queue,
            sizeof(Queue),
            NULL))
        {
            ObDumpShowError(hwndDlg);
            return;
        }

        g_TreeList = 0;
        g_TreeListAtom = 0;
        if (!supInitTreeListForDump(hwndDlg, &g_TreeListAtom, &g_TreeList)) {
            ObDumpShowError(hwndDlg);
            return;
        }

        lpDesc2 = NULL;
        if (Queue.Header.Size == (sizeof(KQUEUE) / sizeof(ULONG))) {
            lpDesc2 = L"sizeof(KQUEUE)/sizeof(ULONG)";
        }

        h_tviRootItem = TreeListAddItem(
            g_TreeList,
            NULL,
            TVIF_TEXT | TVIF_STATE,
            TVIS_EXPANDED,
            TVIS_EXPANDED,
            T_KQUEUE,
            NULL);

        //Header
        ObDumpDispatcherHeader(h_tviRootItem, &Queue.Header, NULL, NULL, lpDesc2);

        //EntryListHead
        ObDumpListEntry(g_TreeList, h_tviRootItem, L"EntryListHead", &Queue.EntryListHead);

        //CurrentCount
        ObDumpUlong(g_TreeList, h_tviRootItem, L"CurrentCount", NULL, Queue.CurrentCount, TRUE, FALSE, 0, 0);

        //MaximumCount
        ObDumpUlong(g_TreeList, h_tviRootItem, L"MaximumCount", NULL, Queue.MaximumCount, TRUE, FALSE, 0, 0);

        //ThreadListHead
        ObDumpListEntry(g_TreeList, h_tviRootItem, L"ThreadListHead", &Queue.ThreadListHead);

    }
    __except (exceptFilter(GetExceptionCode(), GetExceptionInformation())) {
        return;
    }
}

/*
* ObDumpFltServerPort
*
* Purpose:
*
* Dump FLT_SERVER_PORT_OBJECT members to the treelist.
*
*/
VOID ObDumpFltServerPort(
    _In_ PROP_OBJECT_INFO *Context,
    _In_ HWND hwndDlg
)
{
    HTREEITEM h_tviRootItem;
    PRTL_PROCESS_MODULES pModules = NULL;
    FLT_SERVER_PORT_OBJECT FltServerPortObject;

    if (Context == NULL) {
        return;
    }

    __try {
        //dump PortObject
        RtlSecureZeroMemory(&FltServerPortObject, sizeof(FltServerPortObject));

        if (!kdReadSystemMemoryEx(
            Context->ObjectInfo.ObjectAddress,
            &FltServerPortObject,
            sizeof(FltServerPortObject),
            NULL))
        {
            ObDumpShowError(hwndDlg);
            return;
        }

        g_TreeList = 0;
        g_TreeListAtom = 0;
        if (!supInitTreeListForDump(hwndDlg, &g_TreeListAtom, &g_TreeList)) {
            ObDumpShowError(hwndDlg);
            return;
        }

        pModules = (PRTL_PROCESS_MODULES)supGetSystemInfo(SystemModuleInformation);
        if (pModules == NULL) {
            ObDumpShowError(hwndDlg);
            return;
        }

        h_tviRootItem = TreeListAddItem(
            g_TreeList,
            NULL,
            TVIF_TEXT | TVIF_STATE,
            TVIS_EXPANDED,
            TVIS_EXPANDED,
            T_FLT_SERVER_PORT_OBJECT,
            NULL);

        ObDumpListEntry(g_TreeList, h_tviRootItem, L"FilterLink", &FltServerPortObject.FilterLink);

        ObDumpAddressWithModule(h_tviRootItem, L"ConnectNotify",
            FltServerPortObject.ConnectNotify, pModules, NULL, 0);

        ObDumpAddressWithModule(h_tviRootItem, L"DisconnectNotify",
            FltServerPortObject.DisconnectNotify, pModules, NULL, 0);

        ObDumpAddressWithModule(h_tviRootItem, L"MessageNotify",
            FltServerPortObject.MessageNotify, pModules, NULL, 0);

        ObDumpAddress(g_TreeList, h_tviRootItem, L"Filter", T_PFLT_FILTER, FltServerPortObject.Filter, 0, 0);
        ObDumpAddress(g_TreeList, h_tviRootItem, L"Cookie", NULL, FltServerPortObject.Cookie, 0, 0);
        ObDumpUlong(g_TreeList, h_tviRootItem, L"Flags", NULL, FltServerPortObject.Flags, TRUE, FALSE, 0, 0);
        ObDumpUlong(g_TreeList, h_tviRootItem, L"NumberOfConnections", NULL, FltServerPortObject.NumberOfConnections, TRUE, FALSE, 0, 0);
        ObDumpUlong(g_TreeList, h_tviRootItem, L"MaxConnections", NULL, FltServerPortObject.MaxConnections, TRUE, FALSE, 0, 0);

        supHeapFree(pModules);
    }
    __except (exceptFilter(GetExceptionCode(), GetExceptionInformation())) {
        return;
    }
}

/*
* ObDumpAlpcPortCommunicationInfo
*
* Purpose:
*
* Dump ALPC_PORT->CommunicationInfo substructure to the treelist.
*
*/
VOID ObDumpAlpcPortCommunicationInfo(
    _In_ ULONG StructureVersion,
    _In_ ULONG_PTR StructureAddress,
    HTREEITEM h_tviRootItem
)
{
    HTREEITEM h_tviSubItem;
    PBYTE Buffer = NULL;
    ULONG BufferSize = 0;

    union {
        union {
            ALPC_COMMUNICATION_INFO_V1 *CommInfoV1;
            ALPC_COMMUNICATION_INFO_V2 *CommInfoV2;
        } u1;
        PBYTE Ref;
    } AlpcPortCommunicationInfo;

    if ((StructureVersion == 0) || (StructureVersion > 2)) return;

    if (StructureVersion == 1) {
        BufferSize = sizeof(ALPC_COMMUNICATION_INFO_V1);
    }
    else {
        BufferSize = sizeof(ALPC_COMMUNICATION_INFO_V2);
    }

    BufferSize = ALIGN_UP_BY(BufferSize, PAGE_SIZE);
    Buffer = (PBYTE)supVirtualAlloc(BufferSize);
    if (Buffer == NULL)
        return;

    if (!kdReadSystemMemoryEx(
        StructureAddress,
        Buffer,
        BufferSize,
        NULL))
    {
        supVirtualFree(Buffer);
        return;
    }

    AlpcPortCommunicationInfo.Ref = Buffer;

    //
    // Dump version unaffected fields.
    //
    ObDumpAddress(
        g_TreeList,
        h_tviRootItem,
        TEXT("ConnectionPort"),
        T_PALPC_PORT_OBJECT,
        (PVOID)AlpcPortCommunicationInfo.u1.CommInfoV1->ConnectionPort,
        0,
        0);

    ObDumpAddress(
        g_TreeList,
        h_tviRootItem,
        TEXT("ServerCommunicationPort"),
        T_PALPC_PORT_OBJECT,
        (PVOID)AlpcPortCommunicationInfo.u1.CommInfoV1->ServerCommunicationPort,
        0,
        0);

    ObDumpAddress(
        g_TreeList,
        h_tviRootItem,
        TEXT("ClientCommunicationPort"),
        T_PALPC_PORT_OBJECT,
        (PVOID)AlpcPortCommunicationInfo.u1.CommInfoV1->ClientCommunicationPort,
        0,
        0);

    ObDumpListEntry(
        g_TreeList,
        h_tviRootItem,
        TEXT("CommunicationList"),
        &AlpcPortCommunicationInfo.u1.CommInfoV1->CommunicationList);

    //
    //  PALPC_HANDLE_ENTRY dump.
    //
    h_tviSubItem = TreeListAddItem(
        g_TreeList,
        h_tviRootItem,
        TVIF_TEXT | TVIF_STATE,
        TVIS_EXPANDED,
        TVIS_EXPANDED,
        T_ALPC_HANDLE_TABLE,
        NULL);

    ObDumpAddress(
        g_TreeList,
        h_tviSubItem,
        TEXT("Handles"),
        TEXT("PALPC_HANDLE_ENTRY"),
        (PVOID)AlpcPortCommunicationInfo.u1.CommInfoV1->HandleTable.Handles,
        0,
        0);

    ObDumpUlong(
        g_TreeList,
        h_tviSubItem,
        TEXT("TotalHandles"),
        NULL,
        AlpcPortCommunicationInfo.u1.CommInfoV1->HandleTable.TotalHandles,
        TRUE,
        FALSE,
        0,
        0);

    ObDumpUlong(
        g_TreeList,
        h_tviSubItem,
        TEXT("Flags"),
        NULL,
        AlpcPortCommunicationInfo.u1.CommInfoV1->HandleTable.Flags,
        TRUE,
        FALSE,
        0,
        0);

    ObDumpPushLock(
        g_TreeList,
        h_tviSubItem,
        AlpcPortCommunicationInfo.u1.CommInfoV1->HandleTable.Lock.Ptr,
        0,
        0);

    //
    // Version specific field.
    //
    if (StructureVersion == 2) {
        ObDumpAddress(
            g_TreeList,
            h_tviRootItem,
            TEXT("CloseMessage"),
            TEXT("PKALPC_MESSAGE"),
            (PVOID)AlpcPortCommunicationInfo.u1.CommInfoV2->CloseMessage,
            0,
            0);
    }
    supVirtualFree(Buffer);
}

/*
* ObDumpAlpcPort
*
* Purpose:
*
* Dump ALPC_PORT members to the treelist.
*
*/
VOID ObDumpAlpcPort(
    _In_ PROP_OBJECT_INFO *Context,
    _In_ HWND hwndDlg
)
{
    ULONG BufferSize = 0, ObjectVersion = 0, i, c;
    HTREEITEM h_tviRootItem, h_tviSubItem;

    PBYTE PortDumpBuffer = NULL;
    ALPC_PORT_ATTRIBUTES *PortAttributes;
    ALPC_PORT_STATE PortState;
    TL_SUBITEMS_FIXED subitems;

    WCHAR szBuffer[100];

    union {
        union {
            ALPC_PORT_7600 *Port7600;
            ALPC_PORT_9200 *Port9200;
            ALPC_PORT_9600 *Port9600;
            ALPC_PORT_10240 *Port10240;
        } u1;
        PBYTE Ref;
    } AlpcPort;

    PortDumpBuffer = (PBYTE)ObDumpAlpcPortObjectVersionAware(
        Context->ObjectInfo.ObjectAddress,
        &BufferSize,
        &ObjectVersion);

    if (PortDumpBuffer == NULL) {
        ObDumpShowError(hwndDlg);
        return;
    }

    g_TreeList = 0;
    g_TreeListAtom = 0;
    if (!supInitTreeListForDump(hwndDlg, &g_TreeListAtom, &g_TreeList)) {
        ObDumpShowError(hwndDlg);
        supVirtualFree(PortDumpBuffer);
        return;
    }

    AlpcPort.Ref = PortDumpBuffer;

    h_tviRootItem = TreeListAddItem(
        g_TreeList,
        NULL,
        TVIF_TEXT | TVIF_STATE,
        TVIS_EXPANDED,
        TVIS_EXPANDED,
        T_ALPC_PORT_OBJECT,
        NULL);

    //
    //  Dump AlpcPort->PortListEntry, same offset for every supported Windows.
    //   
    ObDumpListEntry(
        g_TreeList,
        h_tviRootItem,
        TEXT("PortListEntry"),
        &AlpcPort.u1.Port7600->PortListEntry);

    //
    //  Dump AlpcPort->CommunicationInfo, same offset for every supported Windows, however target structure is version aware.
    // 

    RtlSecureZeroMemory(&subitems, sizeof(subitems));
    subitems.Count = 2;

    szBuffer[0] = L'0';
    szBuffer[1] = L'x';
    szBuffer[2] = 0;
    u64tohex((ULONG_PTR)AlpcPort.u1.Port7600->CommunicationInfo, &szBuffer[2]);
    subitems.Text[0] = szBuffer;
    subitems.Text[1] = TEXT("PALPC_COMMUNICATION_INFO");

    h_tviSubItem = TreeListAddItem(
        g_TreeList,
        h_tviRootItem,
        TVIF_TEXT,
        0,
        0,
        TEXT("CommunicationInfo"),
        &subitems);

    ObDumpAlpcPortCommunicationInfo(
        (ObjectVersion > 2) ? 2 : 1,
        (ULONG_PTR)AlpcPort.u1.Port7600->CommunicationInfo,
        h_tviSubItem);

    //
    //  Dump AlpcPort->OwnerProcess, same offset for every supported Windows, however target structure is version aware.
    //
    ObDumpAddress(
        g_TreeList,
        h_tviRootItem,
        TEXT("Owner"),
        TEXT("PEPROCESS"),
        (PVOID)AlpcPort.u1.Port7600->OwnerProcess,
        0,
        0);

    //
    //  Dump AlpcPort->CompletionPort, same offset for every supported Windows.
    //
    ObDumpAddress(
        g_TreeList,
        h_tviRootItem,
        TEXT("CompletionPort"),
        NULL,
        (PVOID)AlpcPort.u1.Port7600->CompletionPort,
        0,
        0);

    //
    //  Dump AlpcPort->CompletionKey, same offset for every supported Windows.
    //
    ObDumpAddress(
        g_TreeList,
        h_tviRootItem,
        TEXT("CompletionKey"),
        NULL,
        (PVOID)AlpcPort.u1.Port7600->CompletionKey,
        0,
        0);

    //
    //  Dump AlpcPort->CompletionPacketLookaside, same offset for every supported Windows, however target structure is version aware.
    //
    ObDumpAddress(
        g_TreeList,
        h_tviRootItem,
        TEXT("CompletionPacketLookaside"),
        TEXT("PALPC_COMPLETION_PACKET_LOOKASIDE"),
        (PVOID)AlpcPort.u1.Port7600->CompletionPacketLookaside,
        0,
        0);

    //
    //  Dump AlpcPort->PortContext, same offset for every supported Windows.
    //
    ObDumpAddress(
        g_TreeList,
        h_tviRootItem,
        TEXT("PortContext"),
        NULL,
        (PVOID)AlpcPort.u1.Port7600->PortContext,
        0,
        0);

    //
    //  Dump AlpcPort->StaticSecurity, same offset for every supported Windows.
    //
    /*
    ObDumpSqos(
        g_TreeList,
        h_tviRootItem,
        &AlpcPort.u1.Port7600->StaticSecurity.SecurityQos);
    */

    //
    // Dump AlpcPort->PortAttributes, offset is version aware.
    //
    RtlSecureZeroMemory(&subitems, sizeof(subitems));
    subitems.Count = 2;

    subitems.Text[1] = TEXT("ALPC_PORT_ATTRIBUTES");

    h_tviSubItem = TreeListAddItem(
        g_TreeList,
        h_tviRootItem,
        TVIF_TEXT | TVIF_STATE,
        TVIS_EXPANDED,
        TVIS_EXPANDED,
        TEXT("PortAttributes"),
        &subitems);

    switch (ObjectVersion) {
    case 1:
        PortAttributes = &AlpcPort.u1.Port7600->PortAttributes;
        break;
    case 2:
        PortAttributes = &AlpcPort.u1.Port9200->PortAttributes;
        break;
    case 3:
        PortAttributes = &AlpcPort.u1.Port9600->PortAttributes;
        break;
    case 4:
        PortAttributes = &AlpcPort.u1.Port10240->PortAttributes;
        break;
    default:
        PortAttributes = NULL;
        break;
    }

    if (PortAttributes) {

        ObDumpUlong(
            g_TreeList,
            h_tviSubItem,
            T_FLAGS,
            NULL,
            PortAttributes->Flags,
            TRUE,
            FALSE,
            0,
            0);

        ObDumpSqos(
            g_TreeList,
            h_tviSubItem,
            &PortAttributes->SecurityQos);

        ObDumpUlong64(
            g_TreeList,
            h_tviSubItem,
            TEXT("MaxMessageLength"),
            NULL,
            (ULONG64)PortAttributes->MaxMessageLength,
            FALSE,
            0,
            0);

        ObDumpUlong64(
            g_TreeList,
            h_tviSubItem,
            TEXT("MemoryBandwidth"),
            NULL,
            (ULONG64)PortAttributes->MemoryBandwidth,
            FALSE,
            0,
            0);

        ObDumpUlong64(
            g_TreeList,
            h_tviSubItem,
            TEXT("MaxPoolUsage"),
            NULL,
            (ULONG64)PortAttributes->MaxPoolUsage,
            FALSE,
            0,
            0);

        ObDumpUlong64(
            g_TreeList,
            h_tviSubItem,
            TEXT("MaxSectionSize"),
            NULL,
            (ULONG64)PortAttributes->MaxSectionSize,
            FALSE,
            0,
            0);

        ObDumpUlong64(
            g_TreeList,
            h_tviSubItem,
            TEXT("MaxViewSize"),
            NULL,
            (ULONG64)PortAttributes->MaxViewSize,
            FALSE,
            0,
            0);

        ObDumpUlong64(
            g_TreeList,
            h_tviSubItem,
            TEXT("MaxTotalSectionSize"),
            NULL,
            (ULONG64)PortAttributes->MaxTotalSectionSize,
            FALSE,
            0,
            0);

        ObDumpUlong(
            g_TreeList,
            h_tviSubItem,
            TEXT("DupObjectTypes"),
            NULL,
            PortAttributes->DupObjectTypes,
            FALSE,
            FALSE,
            0,
            0);
    }

    //
    // Dump AlpcPort->State, offset is version aware.
    //
    h_tviSubItem = TreeListAddItem(
        g_TreeList,
        h_tviRootItem,
        TVIF_TEXT,
        0,
        0,
        TEXT("State"),
        NULL);

    PortState.State = 0;

    switch (ObjectVersion) {
    case 1:
        PortState.State = AlpcPort.u1.Port7600->u1.State;
        break;
    case 2:
        PortState.State = AlpcPort.u1.Port9200->u1.State;
        break;
    case 3:
        PortState.State = AlpcPort.u1.Port9600->u1.State;
        break;
    case 4:
        PortState.State = AlpcPort.u1.Port10240->u1.State;
        break;
    default:
        break;
    }

    for (i = 0; i < 16; i++) {
        if (i == 1) {
            c = (BYTE)PortState.s1.Type;
        }
        else {
            c = GET_BIT(PortState.State, i);
        }
        ObDumpByte(
            g_TreeList,
            h_tviSubItem,
            T_ALPC_PORT_STATE[i],
            NULL,
            (BYTE)c,
            0,
            0,
            FALSE);

    }
    supVirtualFree(PortDumpBuffer);
}

/*
* ObDumpCallback
*
* Purpose:
*
* Dump CALLBACK_OBJECT callback members to the treelist.
*
*/
VOID ObDumpCallback(
    _In_ PROP_OBJECT_INFO *Context,
    _In_ HWND hwndDlg
)
{
    SIZE_T Count;
    ULONG_PTR ListHead;
    HTREEITEM h_tviRootItem;

    LIST_ENTRY ListEntry;

    PRTL_PROCESS_MODULES Modules;

    CALLBACK_OBJECT ObjectDump;
    CALLBACK_REGISTRATION CallbackRegistration;

    //
    // Read object body.
    //
    if (!kdReadSystemMemoryEx(
        Context->ObjectInfo.ObjectAddress,
        (PVOID)&ObjectDump,
        sizeof(ObjectDump),
        NULL))
    {
        ObDumpShowError(hwndDlg);
        return;
    }

    //
    // Verify object signature.
    //
    if (ObjectDump.Signature != EX_CALLBACK_SIGNATURE) {
        ObDumpShowError(hwndDlg);
        return;
    }

    //
    // Create a snapshot list of loaded modules.
    //
    Modules = (PRTL_PROCESS_MODULES)supGetSystemInfo(SystemModuleInformation);
    if (Modules == NULL) {
        ObDumpShowError(hwndDlg);
        return;
    }

    //
    // Prepare treelist for output.
    //
    g_TreeList = 0;
    g_TreeListAtom = 0;
    if (!supInitTreeListForDump(hwndDlg, &g_TreeListAtom, &g_TreeList)) {
        ObDumpShowError(hwndDlg);
        return;
    }

    //
    // Add root item to the treelist in expanded state.
    //
    h_tviRootItem = TreeListAddItem(
        g_TreeList,
        NULL,
        TVIF_TEXT | TVIF_STATE,
        TVIS_EXPANDED,
        TVIS_EXPANDED,
        TEXT("Callbacks"),
        NULL);

    //
    // Walk RegisteredCallback list entry.
    //
    ListHead = Context->ObjectInfo.ObjectAddress + FIELD_OFFSET(CALLBACK_OBJECT, RegisteredCallbacks);
    ListEntry.Flink = ObjectDump.RegisteredCallbacks.Flink;
    Count = 0;
    while ((ULONG_PTR)ListEntry.Flink != ListHead) {

        //
        // Read callback registration data.
        //
        if (!kdReadSystemMemoryEx((ULONG_PTR)ListEntry.Flink,
            (PVOID)&CallbackRegistration,
            sizeof(CallbackRegistration),
            NULL))
        {
            //
            // Abort all output on error.
            //
            ObDumpShowError(hwndDlg);
            break;
        }

        Count += 1;
        ListEntry.Flink = CallbackRegistration.Link.Flink;

        ObDumpAddressWithModule(h_tviRootItem,
            Context->lpObjectName,
            CallbackRegistration.CallbackFunction,
            Modules,
            NULL,
            0);
    }

    //
    // If nothing found (or possible query error) output this message.
    //
    if (Count == 0) {
        ObDumpShowMessage(hwndDlg,
            TEXT("This object has no registered callbacks or there is an query error."));
    }

    supHeapFree(Modules);
}

/*
* ObjectDumpHandlePopupMenu
*
* Purpose:
*
* Object dump popup construction
*
*/
VOID ObjectDumpHandlePopupMenu(
    _In_ HWND hwndDlg
)
{
    POINT pt1;
    HMENU hMenu;

    if (GetCursorPos(&pt1) == FALSE)
        return;

    hMenu = CreatePopupMenu();
    if (hMenu) {
        InsertMenu(hMenu, 0, MF_BYCOMMAND, ID_OBJECT_COPY, T_COPYVALUE);
        InsertMenu(hMenu, 1, MF_BYCOMMAND, ID_ADDINFO_COPY, T_COPYADDINFO);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN, pt1.x, pt1.y, 0, hwndDlg, NULL);
        DestroyMenu(hMenu);
    }
}

/*
* ObjectDumpInitDialog
*
* Purpose:
*
* Object window WM_INITDIALOG handler.
*
* Show load banner and proceed with actual info dump.
*
*/
INT_PTR ObjectDumpInitDialog(
    _In_ HWND hwndDlg,
    _In_ LPARAM lParam
)
{
    PROP_OBJECT_INFO *Context = NULL;
    PROPSHEETPAGE    *pSheet = (PROPSHEETPAGE *)lParam;
#ifndef _DEBUG
    HWND hwndBanner = supDisplayLoadBanner(hwndDlg,
        TEXT("Processing object dump, please wait"));
#endif
    __try {
        Context = (PROP_OBJECT_INFO*)pSheet->lParam;
        if (Context) {

            switch (Context->TypeIndex) {

            case ObjectTypeDirectory:
                ObDumpDirectoryObject(Context, hwndDlg);
                break;

            case ObjectTypeDriver:
                ObDumpDriverObject(Context, hwndDlg);
                break;

            case ObjectTypeDevice:
                ObDumpDeviceObject(Context, hwndDlg);
                break;

            case ObjectTypeEvent:
            case ObjectTypeMutant:
            case ObjectTypeSemaphore:
            case ObjectTypeTimer:
                ObDumpSyncObject(Context, hwndDlg);
                break;

            case ObjectTypePort:
                ObDumpAlpcPort(Context, hwndDlg);
                break;

            case ObjectTypeIoCompletion:
                ObDumpQueueObject(Context, hwndDlg);
                break;

            case ObjectTypeFltConnPort:
                ObDumpFltServerPort(Context, hwndDlg);
                break;

            case ObjectTypeCallback:
                ObDumpCallback(Context, hwndDlg);
                break;

            case ObjectTypeType:
                ObDumpObjectType(Context, hwndDlg);
                break;
            }
        }
    }
    __finally {
#ifndef _DEBUG
        SendMessage(hwndBanner, WM_CLOSE, 0, 0);
#endif
    }

    return 1;
}

/*
* ObjectDumpDialogProc
*
* Purpose:
*
* Object window procedure and object dump select.
*
*/
INT_PTR CALLBACK ObjectDumpDialogProc(
    _In_  HWND hwndDlg,
    _In_  UINT uMsg,
    _In_  WPARAM wParam,
    _In_  LPARAM lParam
)
{
    switch (uMsg) {

    case WM_CONTEXTMENU:
        ObjectDumpHandlePopupMenu(hwndDlg);
        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {
        case ID_OBJECT_COPY:
            supCopyTreeListSubItemValue(g_TreeList, 0);
            break;
        case ID_ADDINFO_COPY:
            supCopyTreeListSubItemValue(g_TreeList, 1);
            break;
        default:
            break;
        }
        break;

    case WM_DESTROY:
        DestroyWindow(g_TreeList);
        UnregisterClass(MAKEINTATOM(g_TreeListAtom), g_WinObj.hInstance);
        break;

    case WM_INITDIALOG:

        return ObjectDumpInitDialog(
            hwndDlg,
            lParam);

    }
    return 0;
}

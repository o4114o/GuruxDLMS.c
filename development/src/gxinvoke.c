//
// --------------------------------------------------------------------------
//  Gurux Ltd
//
//
//
// Filename:        $HeadURL$
//
// Version:         $Revision$,
//                  $Date$
//                  $Author$
//
// Copyright (c) Gurux Ltd
//
//---------------------------------------------------------------------------
//
//  DESCRIPTION
//
// This file is a part of Gurux Device Framework.
//
// Gurux Device Framework is Open Source software; you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; version 2 of the License.
// Gurux Device Framework is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// This code is licensed under the GNU General Public License v2.
// Full text may be retrieved at http://www.gnu.org/licenses/gpl-2.0.txt
//---------------------------------------------------------------------------

#include "../include/gxignore.h"
#ifndef DLMS_IGNORE_SERVER

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
#include <assert.h>
#endif

#include "../include/gxmem.h"
#if _MSC_VER > 1400
#include <crtdbg.h>
#endif
#include <string.h>

#include "../include/cosem.h"
#include "../include/dlms.h"
#include "../include/gxset.h"
#include "../include/gxinvoke.h"
#include "../include/helpers.h"
#include "../include/objectarray.h"
#include "../include/ciphering.h"
#include "../include/gxget.h"
#include "../include/gxkey.h"
#include "../include/serverevents.h"

#ifndef DLMS_IGNORE_CHARGE

int invoke_Charge(
    gxCharge* object,
    unsigned char index,
    dlmsVARIANT* value)
{
    gxChargeTable* ct, * it;
    int ret = 0, pos;
    //Update unit charge.
    if (index == 1)
    {
        for (pos = 0; pos != object->unitChargePassive.chargeTables.size; ++pos)
        {
#ifdef DLMS_IGNORE_MALLOC
            ret = arr_getByIndex(&object->unitChargePassive.chargeTables, pos, (void**)&ct, sizeof(gxCharge));
#else
            ret = arr_getByIndex(&object->unitChargePassive.chargeTables, pos, (void**)&ct);
#endif //DLMS_IGNORE_MALLOC
            if (ret != 0)
            {
                return ret;
            }
            ct->chargePerUnit = (short)var_toInteger(value);
        }
    }
    //Activate passive unit charge.
    else if (index == 2)
    {
        object->unitChargeActive.chargePerUnitScaling = object->unitChargePassive.chargePerUnitScaling;
        object->unitChargeActive.commodity = object->unitChargePassive.commodity;
        for (pos = 0; pos != object->unitChargePassive.chargeTables.size; ++pos)
        {
#ifdef DLMS_IGNORE_MALLOC
            if ((ret = arr_getByIndex(&object->unitChargePassive.chargeTables, pos, (void**)&ct, sizeof(gxChargeTable))) != 0 ||
                (ret = arr_getByIndex(&object->unitChargeActive.chargeTables, object->unitChargeActive.chargeTables.size, (void**)&it, sizeof(gxChargeTable))) != 0)
            {
                break;
            }
            ++object->unitChargeActive.chargeTables.size;
#else
            if ((ret = arr_getByIndex(&object->unitChargePassive.chargeTables, pos, (void**)&ct)) != 0)
            {
                break;
            }
            it = (gxChargeTable*)gxmalloc(sizeof(gxChargeTable));
            arr_push(&object->unitChargeActive.chargeTables, it);
#endif //DLMS_IGNORE_MALLOC
            it->chargePerUnit = ct->chargePerUnit;
            it->index = ct->index;
        }
    }
    //Update total amount remaining.
    else if (index == 4)
    {
        object->totalAmountRemaining += var_toInteger(value);
    }
    else
    {
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}
#endif //DLMS_IGNORE_CHARGE
#ifndef DLMS_IGNORE_CREDIT
int invoke_Credit(
    gxCredit* object,
    unsigned char index,
    dlmsVARIANT* value)
{
    int ret = 0;
    //Update amount.
    if (index == 1)
    {
        object->currentCreditAmount += var_toInteger(value);
    }
    //Set amount to value.
    else if (index == 2)
    {
        object->currentCreditAmount = var_toInteger(value);
    }
    else if (index == 3)
    {
        // The mechanism for selecting the �Credit� is not in the scope of COSEM and
        // shall be specified by the implementer (e.g. button push, meter process, script etc.).
        object->status |= 4;
    }
    else
    {
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}
#endif //DLMS_IGNORE_CREDIT
#ifndef DLMS_IGNORE_TOKEN_GATEWAY
int invoke_gxTokenGateway(
    gxTokenGateway* object,
    unsigned char index,
    dlmsVARIANT* value)
{
    int ret = 0;
    //Update token.
    if (index == 1)
    {
        bb_clear(&object->token);
        if (value->vt == DLMS_DATA_TYPE_OCTET_STRING)
        {
            bb_set(&object->token, value->byteArr->data, value->byteArr->size);
        }
    }
    else
    {
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}
#endif //DLMS_IGNORE_TOKEN_GATEWAY

int invoke_AssociationLogicalName(
    dlmsServerSettings* settings,
    gxValueEventArg* e)
{
    int ret = 0;
    gxAssociationLogicalName* object = (gxAssociationLogicalName*)e->target;
    // Check reply_to_HLS_authentication
    if (e->index == 1)
    {
        unsigned char ch;
#ifndef DLMS_IGNORE_HIGH_GMAC
        gxByteBuffer tmp;
#endif //DLMS_IGNORE_HIGH_GMAC
        unsigned char tmp2[64];
        unsigned char equal;
        unsigned long ic = 0;
        gxByteBuffer bb;
        gxByteBuffer* readSecret;
        bb_init(&bb);
#ifdef DLMS_IGNORE_MALLOC
        unsigned short count;
        if ((ret = bb_getUInt8(e->parameters.byteArr, &ch)) != 0)
        {
            return ret;
        }
        if (ch != DLMS_DATA_TYPE_OCTET_STRING)
        {
            return DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
        if ((ret = hlp_getObjectCount2(e->parameters.byteArr, &count)) != 0)
        {
            return ret;
        }
        if (count > bb_available(e->parameters.byteArr))
        {
            return DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
#endif //DLMS_IGNORE_MALLOC

#ifndef DLMS_IGNORE_HIGH_GMAC
        if (settings->base.authentication == DLMS_AUTHENTICATION_HIGH_GMAC)
        {
            bb_attach(&tmp, settings->base.sourceSystemTitle, sizeof(settings->base.sourceSystemTitle), sizeof(settings->base.sourceSystemTitle));
            if ((ret = bb_getUInt8(e->parameters.byteArr, &ch)) != 0 ||
                (ret = bb_getUInt32(e->parameters.byteArr, &ic)) != 0)
            {
                return ret;
            }
            //IC is also compared.
            e->parameters.byteArr->position -= 5;
            readSecret = &tmp;
        }
        else
#endif //DLMS_IGNORE_MALLOC
        {
            readSecret = &object->secret;
        }

        bb_init(&bb);
        bb_attach(&bb, tmp2, 0, sizeof(tmp2));
        if ((ret = dlms_secure(&settings->base,
            ic,
            &settings->base.stoCChallenge,
            readSecret,
            &bb)) != 0)
        {
            return ret;
        }
        equal = bb_available(e->parameters.byteArr) != 0 &&
            bb_compare(&bb,
                e->parameters.byteArr->data + e->parameters.byteArr->position,
                bb_available(e->parameters.byteArr));
        bb_clear(&settings->info.data);
        if (equal)
        {
#ifndef DLMS_IGNORE_HIGH_GMAC
            if (settings->base.authentication == DLMS_AUTHENTICATION_HIGH_GMAC)
            {
#ifdef DLMS_IGNORE_MALLOC
                bb_attach(&tmp, settings->base.cipher.systemTitle, sizeof(settings->base.cipher.systemTitle), sizeof(settings->base.cipher.systemTitle));
                readSecret = &tmp;
#else
                readSecret = &settings->base.cipher.systemTitle;
#endif //DLMS_IGNORE_MALLOC
                ic = settings->base.cipher.invocationCounter;
            }
#endif //DLMS_IGNORE_HIGH_GMAC
            e->byteArray = 1;
            if ((ret = dlms_secure(&settings->base,
                ic,
                &settings->base.ctoSChallenge,
                readSecret,
                &settings->info.data)) != 0)
            {
                return ret;
            }
            bb_insertUInt8(&settings->info.data, 0, DLMS_DATA_TYPE_OCTET_STRING);
            bb_insertUInt8(&settings->info.data, 1, (unsigned char)(settings->info.data.size - 1));
            object->associationStatus = DLMS_ASSOCIATION_STATUS_ASSOCIATED;
        }
        else
        {
            object->associationStatus = DLMS_ASSOCIATION_STATUS_NON_ASSOCIATED;
        }
    }
    else if (e->index == 2)
    {
        if (e->parameters.byteArr->size == 0)
        {
            ret = DLMS_ERROR_CODE_READ_WRITE_DENIED;
        }
        else
        {
            bb_clear(&object->secret);
            bb_set(&object->secret, e->parameters.byteArr->data, e->parameters.byteArr->size);
        }
    }
    else if (e->index == 5)
    {
#ifdef DLMS_IGNORE_MALLOC
        gxUser* it;
        if (!(object->userList.size < arr_getCapacity(&object->userList)))
        {
            ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
        }
        else
        {
            ++object->userList.size;
            if ((ret = arr_getByIndex(&object->userList, object->userList.size - 1, (void**)&it, sizeof(gxUser))) == 0)
            {
                if ((ret = cosem_checkStructure(e->parameters.byteArr, 2)) != 0 ||
                    (ret = cosem_getUInt8(e->parameters.byteArr, &it->id)) != 0 ||
                    (ret = cosem_getString2(e->parameters.byteArr, it->name, sizeof(it->name))) != 0)
                {
                    //Error code is returned at the end of the function.
                }
            }
        }
#else
        if (e->parameters.vt != DLMS_DATA_TYPE_STRUCTURE || e->parameters.Arr->size != 2)
        {
            ret = DLMS_ERROR_CODE_READ_WRITE_DENIED;
        }
        else
        {
            dlmsVARIANT* tmp;
            char* name;
            gxKey2* it = (gxKey2*)gxmalloc(sizeof(gxKey2));
            if ((ret = va_getByIndex(e->parameters.Arr, 0, &tmp)) != 0)
            {
                gxfree(it);
                return ret;
            }
            it->key = tmp->bVal;
            if ((ret = va_getByIndex(e->parameters.Arr, 1, &tmp)) != 0)
            {
                gxfree(it);
                return ret;
            }
            name = gxmalloc(tmp->strVal->size + 1);
            it->value = name;
            *(name + tmp->strVal->size) = '\0';
            memcpy(it->value, tmp->strVal->data, tmp->strVal->size);
            ret = arr_push(&((gxAssociationLogicalName*)e->target)->userList, it);
        }
#endif //DLMS_IGNORE_MALLOC
    }
    else if (e->index == 6)
    {
        int pos;
#ifdef DLMS_IGNORE_MALLOC
        gxUser* it;
        unsigned char id;
        char name[MAX_SAP_ITEM_NAME_LENGTH];
        if ((ret = cosem_checkStructure(e->parameters.byteArr, 2)) == 0 &&
            (ret = cosem_getUInt8(e->parameters.byteArr, &id)) == 0 &&
            (ret = cosem_getString2(e->parameters.byteArr, name, sizeof(name))) == 0)
        {
            unsigned short size = (unsigned short)strlen(name);
            for (pos = 0; pos != ((gxAssociationLogicalName*)e->target)->userList.size; ++pos)
            {
                if ((ret = arr_getByIndex(&((gxAssociationLogicalName*)e->target)->userList, pos, (void**)&it, sizeof(gxUser))) != 0)
                {
                    return ret;
                }
                if (it->id == id && memcmp(it->name, name, size) == 0)
                {
                    int pos2;
                    gxUser* it2;
                    for (pos2 = pos + 1; pos2 < ((gxAssociationLogicalName*)e->target)->userList.size; ++pos2)
                    {
                        if ((ret = arr_getByIndex(&((gxAssociationLogicalName*)e->target)->userList, pos2, (void**)&it2, sizeof(gxUser))) != 0)
                        {
                            break;
                        }
                        memcpy(it, it2, sizeof(gxUser));
                        it = it2;
                    }
                    --((gxAssociationLogicalName*)e->target)->userList.size;
                    break;
                }
            }
        }
#else
        if (e->parameters.Arr->size != 2)
        {
            return DLMS_ERROR_CODE_READ_WRITE_DENIED;
        }
        else
        {
            gxKey2* it;
            unsigned char id;
            dlmsVARIANT* tmp;
            unsigned char* name;
            ret = va_getByIndex(e->parameters.Arr, 0, &tmp);
            id = tmp->bVal;
            ret = va_getByIndex(e->parameters.Arr, 1, &tmp);
            name = tmp->strVal->data;
            int len = tmp->strVal->size;
            for (pos = 0; pos != ((gxAssociationLogicalName*)e->target)->userList.size; ++pos)
            {
                ret = arr_getByIndex(&((gxAssociationLogicalName*)e->target)->userList, pos, (void**)&it);
                if (ret != 0)
                {
                    return ret;
                }
                if (it->key == id && memcmp(it->value, name, len) == 0)
                {
                    arr_removeByIndex(&((gxAssociationLogicalName*)e->target)->userList, pos, (void**)&it);
                    gxfree(it->value);
                    gxfree(it);
                    break;
                }
            }
        }
#endif //DLMS_IGNORE_MALLOC
    }
    else
    {
        ret = DLMS_ERROR_CODE_READ_WRITE_DENIED;
    }
    return ret;
}

#ifndef DLMS_IGNORE_IMAGE_TRANSFER
int invoke_ImageTransfer(
    gxImageTransfer* target,
    gxValueEventArg* e)
{
    int pos, ret = 0;
    //Image transfer initiate
    if (e->index == 1)
    {
        gxByteBuffer tmp;
        gxImageActivateInfo* it, * item = NULL;
        target->imageFirstNotTransferredBlockNumber = 0;
        ba_clear(&target->imageTransferredBlocksStatus);

        dlmsVARIANT* imageIdentifier, * size;
        if ((ret = va_getByIndex(e->parameters.Arr, 0, &imageIdentifier)) != 0 ||
            (ret = va_getByIndex(e->parameters.Arr, 1, &size)) != 0)
        {
            e->error = DLMS_ERROR_CODE_HARDWARE_FAULT;
            return ret;
        }
        target->imageTransferStatus = DLMS_IMAGE_TRANSFER_STATUS_INITIATED;
        unsigned char exists = 0;
        for (pos = 0; pos != target->imageActivateInfo.size; ++pos)
        {
#ifdef DLMS_IGNORE_MALLOC
            if ((ret = arr_getByIndex(&target->imageActivateInfo, pos, (void**)&it, sizeof(gxImageActivateInfo))) != 0)
            {
                return ret;
            }
#else
            if ((ret = arr_getByIndex(&target->imageActivateInfo, pos, (void**)&it)) != 0)
            {
                return ret;
            }
#endif //DLMS_IGNORE_MALLOC
            bb_attach(&tmp, it->identification.data, it->identification.size, it->identification.size);
            if (bb_size(imageIdentifier->byteArr) != 0 &&
                bb_compare(&tmp, imageIdentifier->byteArr->data, imageIdentifier->byteArr->size))
            {
                item = it;
                exists = 1;
            }
        }
        if (!exists)
        {
#ifdef DLMS_IGNORE_MALLOC
            if ((ret = arr_getByIndex(&target->imageActivateInfo, target->imageActivateInfo.size, (void**)&item, sizeof(gxImageActivateInfo))) != 0)
            {
                return ret;
            }
            ++target->imageActivateInfo.size;
#else
            item = (gxImageActivateInfo*)gxmalloc(sizeof(gxImageActivateInfo));
            arr_push(&target->imageActivateInfo, item);
#endif //DLMS_IGNORE_MALLOC
            item->size = var_toInteger(size);
            item->identification.size = 0;
            if ((ret = cosem_getOctectString2(imageIdentifier->byteArr, item->identification.data, sizeof(item->identification.data), (unsigned short*)&item->identification.size)) != 0)
            {
                return ret;
            }
            int cnt = item->size / target->imageBlockSize;
            if (item->size % target->imageBlockSize != 0)
            {
                ++cnt;
            }
#ifndef GX_DLMS_MICROCONTROLLER
            target->imageTransferredBlocksStatus.position = 0;
#endif //GX_DLMS_MICROCONTROLLER
            target->imageTransferredBlocksStatus.size = 0;
            ba_capacity(&target->imageTransferredBlocksStatus, (unsigned short)cnt);
            for (pos = 0; pos != cnt; ++pos)
            {
                ba_set(&target->imageTransferredBlocksStatus, 0);
            }
        }
    }
    //Image block transfer
    else if (e->index == 2)
    {
        dlmsVARIANT* imageIndex;
        if ((ret = va_getByIndex(e->parameters.Arr, 0, &imageIndex)) != 0)
        {
            e->error = DLMS_ERROR_CODE_HARDWARE_FAULT;
            return ret;
        }
        ba_setByIndex(&target->imageTransferredBlocksStatus, var_toInteger(imageIndex), 1);
        target->imageFirstNotTransferredBlockNumber = var_toInteger(imageIndex) + 1;
        target->imageTransferStatus = DLMS_IMAGE_TRANSFER_STATUS_INITIATED;
    }
    //Image verify
    else if (e->index == 3)
    {
    }
    //Image activate.
    else if (e->index == 4)
    {
    }
    else
    {
        ret = DLMS_ERROR_CODE_READ_WRITE_DENIED;
    }
    return ret;
}
#endif //DLMS_IGNORE_IMAGE_TRANSFER

#ifndef DLMS_IGNORE_SAP_ASSIGNMENT
int invoke_SapAssigment(
    gxSapAssignment* target,
    gxValueEventArg* e)
{
    int pos, ret = 0;
    unsigned short id;
    gxSapItem* it;
    //Image transfer initiate
    if (e->index == 1)
    {
#ifdef DLMS_IGNORE_MALLOC
        if ((ret = cosem_checkStructure(e->value.byteArr, 2)) == 0 &&
            (ret = cosem_getUInt16(e->value.byteArr, &id)) == 0)
        {
            if (id == 0)
            {
                unsigned short size;
                unsigned char name[MAX_SAP_ITEM_NAME_LENGTH];
                if ((ret = cosem_getOctectString2(e->value.byteArr, name, sizeof(name), &size)) == 0)
                {
                    name[size] = 0;
                    for (pos = 0; pos != target->sapAssignmentList.size; ++pos)
                    {
                        if ((ret = arr_getByIndex(&target->sapAssignmentList, pos, (void**)&it, sizeof(gxSapItem))) != 0)
                        {
                            return ret;
                        }
                        if (it->name.size == size && memcmp(it->name.value, name, size) == 0)
                        {
                            if ((ret = arr_removeByIndex(&target->sapAssignmentList, pos, sizeof(gxSapItem))) == 0)
                            {
                                --target->sapAssignmentList.size;
                            }
                        }
                    }
                }
            }
            else
            {
                if ((ret = arr_getByIndex(&target->sapAssignmentList, target->sapAssignmentList.size, (void**)&it, sizeof(gxSapItem))) == 0 &&
                    (ret = cosem_getOctectString2(e->value.byteArr, it->name.value, sizeof(it->name.value), &it->name.size)) == 0)
                {
                    it->id = id;
                    ++target->sapAssignmentList.size;
                }
            }
        }
#else
        dlmsVARIANT* tmp;
        if ((ret = va_getByIndex(e->parameters.Arr, 0, &tmp)) != 0)
        {
            return ret;
        }
        id = tmp->uiVal;
        if ((ret = va_getByIndex(e->parameters.Arr, 1, &tmp)) != 0)
        {
            return ret;
        }

        gxByteBuffer* name;
        if (tmp->vt == DLMS_DATA_TYPE_OCTET_STRING)
        {
            name = tmp->byteArr;
        }
        else
        {
            name = tmp->strVal;
        }
        if (id == 0)
        {
            for (pos = 0; pos != target->sapAssignmentList.size; ++pos)
            {
                if ((ret = arr_getByIndex(&target->sapAssignmentList, pos, (void**)&it)) != 0)
                {
                    return ret;
                }
                it->name.position = 0;
                if (name != NULL && bb_compare(&it->name, name->data, bb_size(name)))
                {
                    ret = arr_removeByIndex(&target->sapAssignmentList, pos, (void**)&it);
                    bb_clear(&it->name);
                    gxfree(it);
                    return ret;
                }
            }
        }
        else
        {
            it = (gxSapItem*)gxmalloc(sizeof(gxSapItem));
            it->id = id;
            bb_init(&it->name);
            bb_set(&it->name, name->data, name->size);
            arr_push(&target->sapAssignmentList, it);
        }
#endif //DLMS_IGNORE_MALLOC
    }
    else
    {
        ret = DLMS_ERROR_CODE_READ_WRITE_DENIED;
    }
    return ret;
}
#endif //DLMS_IGNORE_SAP_ASSIGNMENT

#ifndef DLMS_IGNORE_ASSOCIATION_SHORT_NAME
int invoke_AssociationShortName(
    dlmsServerSettings* settings,
    gxValueEventArg* e)
{
    int ret;
    unsigned char equal;
    unsigned long ic = 0;
#ifndef DLMS_IGNORE_HIGH_GMAC
    gxByteBuffer bb;
#endif //DLMS_IGNORE_HIGH_GMAC
    gxByteBuffer* readSecret;
    // Check reply_to_HLS_authentication
    if (e->index == 8)
    {
#ifndef DLMS_IGNORE_HIGH_GMAC
        if (settings->base.authentication == DLMS_AUTHENTICATION_HIGH_GMAC)
        {
            unsigned char ch;
            bb_attach(&bb, settings->base.sourceSystemTitle, sizeof(settings->base.sourceSystemTitle), sizeof(settings->base.sourceSystemTitle));
            readSecret = &bb;
            if ((ret = bb_getUInt8(&settings->info.data, &ch)) != 0 ||
                (ret = bb_getUInt32(&settings->info.data, &ic)) != 0)
            {
                bb_clear(&settings->info.data);
                return ret;
            }
        }
        else
#endif //DLMS_IGNORE_HIGH_GMAC
        {
            readSecret = &((gxAssociationShortName*)e->target)->secret;
        }
        bb_clear(&settings->info.data);
        if ((ret = dlms_secure(&settings->base, ic,
            &settings->base.stoCChallenge, readSecret, &settings->info.data)) != 0)
        {
            bb_clear(&settings->info.data);
            return ret;
        }
        equal = bb_size(e->parameters.byteArr) != 0 &&
            bb_compare(&settings->info.data,
                e->parameters.byteArr->data,
                e->parameters.byteArr->size);
        bb_clear(&settings->info.data);
        if (equal)
        {
            e->byteArray = 1;
#ifndef DLMS_IGNORE_HIGH_GMAC
            if (settings->base.authentication == DLMS_AUTHENTICATION_HIGH_GMAC)
            {
#ifdef DLMS_IGNORE_MALLOC
                bb_attach(&bb, settings->base.cipher.systemTitle, sizeof(settings->base.cipher.systemTitle), sizeof(settings->base.cipher.systemTitle));
                readSecret = &bb;
#else
                readSecret = &settings->base.cipher.systemTitle;
#endif //DLMS_IGNORE_MALLOC
                ic = settings->base.cipher.invocationCounter;
            }
#endif //DLMS_IGNORE_HIGH_GMAC
            if ((ret = dlms_secure(&settings->base,
                ic,
                &settings->base.ctoSChallenge,
                readSecret,
                &settings->info.data)) != 0)
            {
                return ret;
            }
            bb_insertUInt8(&settings->info.data, 0, DLMS_DATA_TYPE_OCTET_STRING);
            bb_insertUInt8(&settings->info.data, 1, (unsigned char)(settings->info.data.size - 1));
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return DLMS_ERROR_CODE_READ_WRITE_DENIED;
    }
    return 0;
}
#endif //DLMS_IGNORE_ASSOCIATION_SHORT_NAME
#ifndef DLMS_IGNORE_SCRIPT_TABLE
int invoke_ScriptTable(dlmsServerSettings* settings, gxValueEventArg* e)
{
    gxScript* s;
    gxScriptAction* sa;
    int ret = 0, pos, pos2;
    unsigned char id = var_toInteger(&e->parameters);
    //Find index and execute it.
    if (e->index == 1)
    {
        for (pos = 0; pos != ((gxScriptTable*)e->target)->scripts.size; ++pos)
        {
#ifdef DLMS_IGNORE_MALLOC
            if ((ret = arr_getByIndex(&((gxScriptTable*)e->target)->scripts, pos, (void**)&s, sizeof(gxScript))) != 0)
#else
            if ((ret = arr_getByIndex(&((gxScriptTable*)e->target)->scripts, pos, (void**)&s)) != 0)
#endif //DLMS_IGNORE_MALLOC
            {
                break;
            }
            if (s->id == id)
            {
                for (pos2 = 0; pos2 != s->actions.size; ++pos2)
                {
#ifdef DLMS_IGNORE_MALLOC
                    if ((ret = arr_getByIndex(&s->actions, pos2, (void**)&sa, sizeof(gxScriptAction))) != 0)
#else
                    if ((ret = arr_getByIndex(&s->actions, pos2, (void**)&sa)) != 0)
#endif //DLMS_IGNORE_MALLOC
                    {
                        break;
                    }
                    gxValueEventArg e2;
                    ve_init(&e2);
                    e2.parameters = sa->parameter;
                    e2.index = sa->index;
#ifndef DLMS_IGNORE_OBJECT_POINTERS
                    e2.target = sa->target;
#else
                    if ((ret = oa_findByLN(&settings->base.objects, sa->objectType, sa->logicalName, &e2.target)) != 0)
                    {
                        break;
                    }
#endif //DLMS_IGNORE_OBJECT_POINTERS
                    if (sa->type == DLMS_SCRIPT_ACTION_TYPE_WRITE)
                    {
                        if ((ret = cosem_setValue(&settings->base, &e2)) != 0)
                        {
                            break;
                        }
                    }
                    else if (sa->type == DLMS_SCRIPT_ACTION_TYPE_EXECUTE)
                    {
                        if ((ret = cosem_invoke(settings, &e2)) != 0)
                        {
                            break;
                        }
                    }
                    else
                    {
                        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}
#endif //DLMS_IGNORE_SCRIPT_TABLE

#ifndef DLMS_IGNORE_ZIG_BEE_NETWORK_CONTROL
int invoke_zigbeeNetworkControl(gxZigBeeNetworkControl* object, unsigned char index, dlmsVARIANT* value)
{
    int ret = 0, pos;
#ifndef DLMS_IGNORE_MALLOC
    dlmsVARIANT* it;
#endif //DLMS_IGNORE_MALLOC
    gxActiveDevice* ad;
    //Register device.
    if (index == 1)
    {
#ifdef DLMS_IGNORE_MALLOC
        if ((ret = arr_getByIndex(&object->activeDevices, object->activeDevices.size, (void**)&ad, sizeof(gxActiveDevice))) == 0)
        {
            ++object->activeDevices.size;
            bb_init(&ad->macAddress);
            ba_init(&ad->status);
            ret = cosem_getOctectString(value->byteArr, &ad->macAddress);
        }
#else
        if ((ret = va_getByIndex(value->Arr, 0, &it)) == 0)
        {
            ad = (gxActiveDevice*)gxcalloc(1, sizeof(gxActiveDevice));
            bb_init(&ad->macAddress);
            ba_init(&ad->status);
            bb_set(&ad->macAddress, it->byteArr->data, it->byteArr->size);
            arr_push(&object->activeDevices, ad);
        }
#endif //DLMS_IGNORE_MALLOC
    }
    //Unregister device.
    else if (index == 2)
    {
        for (pos = 0; pos != object->activeDevices.size; ++pos)
        {
#ifdef DLMS_IGNORE_MALLOC
            if ((ret = arr_getByIndex(&object->activeDevices, pos, (void**)&ad, sizeof(gxActiveDevice))) != 0)
            {
                break;
            }
            if (memcpy(&ad->macAddress, &value->byteArr, 8) == 0)
            {
                int pos2;
                gxActiveDevice* ad2;
                for (pos2 = pos + 1; pos2 < object->activeDevices.size; ++pos2)
                {
                    if ((ret = arr_getByIndex(&object->activeDevices, pos2, (void**)&ad2, sizeof(gxActiveDevice))) != 0)
                    {
                        break;
                    }
                    memcpy(ad, ad2, sizeof(gxActiveDevice));
                    ad = ad2;
                }
                --object->activeDevices.size;
                break;
            }
#else
            ret = arr_getByIndex(&object->activeDevices, pos, (void**)&ad);
            if (ret != 0)
            {
                return ret;
            }
            if (memcpy(&ad->macAddress, &value->byteArr, 8) == 0)
            {
                ret = arr_removeByIndex(&object->activeDevices, pos, (void**)&ad);
                if (ret != 0)
                {
                    return ret;
                }
                gxfree(ad);
                break;
            }
#endif //DLMS_IGNORE_MALLOC
        }
    }
    //Unregister all device.
    else if (index == 3)
    {
        ret = obj_clearActiveDevices(&object->activeDevices);
    }
    //backup PAN
    else if (index == 4)
    {
    }
    //Restore PAN,
    else if (index == 5)
    {
        // This method instructs the coordinator to restore a PAN using backup information.
        // The storage location of the back-up is not currently defined and is an internal function
        // of the DLMS/COSEM Server.
    }
    else
    {
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}
#endif //DLMS_IGNORE_ZIG_BEE_NETWORK_CONTROL
#ifndef DLMS_IGNORE_EXTENDED_REGISTER
int invoke_ExtendedRegister(
    gxExtendedRegister* object,
    unsigned char index)
{
    int ret = 0;
    //Reset.
    if (index == 1)
    {
        ret = var_clear(&object->value);
        if (ret != 0)
        {
            return ret;
        }
        ret = var_clear(&object->status);
    }
    else
    {
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}
#endif //DLMS_IGNORE_EXTENDED_REGISTER
#ifndef DLMS_IGNORE_CLOCK
int invoke_Clock(gxClock* object, unsigned char index, dlmsVARIANT* value)
{
    int ret = 0;
#ifndef DLMS_IGNORE_MALLOC
    dlmsVARIANT* it;
    dlmsVARIANT tmp;
#endif //DLMS_IGNORE_MALLOC
    // Resets the value to the default value.
    // The default value is an instance specific constant.
    if (index == 1)
    {
#ifdef DLMS_USE_EPOCH_TIME
        int minutes = time_getMinutes(&object->time);
#else
        int minutes = object->time.value.tm_min;
#endif // DLMS_USE_EPOCH_TIME
        if (minutes < 8)
        {
            minutes = 0;
        }
        else if (minutes < 23)
        {
            minutes = 15;
        }
        else if (minutes < 38)
        {
            minutes = 30;
        }
        else if (minutes < 53)
        {
            minutes = 45;
        }
        else
        {
            minutes = 0;
            time_addHours(&object->time, 1);
        }
#ifdef DLMS_USE_EPOCH_TIME
        time_addTime(&object->time, 0, -time_getMinutes(&object->time) + minutes, -time_getSeconds(&object->time));
#else
        time_addTime(&object->time, 0, -object->time.value.tm_min + minutes, -object->time.value.tm_sec);
#endif // DLMS_USE_EPOCH_TIME
    }
    // Sets the meter's time to the nearest minute.
    else if (index == 3)
    {
#ifdef DLMS_USE_EPOCH_TIME
        int s = time_getSeconds(&object->time);
#else
        int s = object->time.value.tm_sec;
#endif // DLMS_USE_EPOCH_TIME
        if (s > 30)
        {
            time_addTime(&object->time, 0, 1, 0);
        }
        time_addTime(&object->time, 0, 0, -s);
    }
    //Adjust to preset time.
    else if (index == 4)
    {
        object->time = object->presetTime;
    }
    // Presets the time to a new value (preset_time) and defines
    // avalidity_interval within which the new time can be activated.
    else if (index == 5)
    {
#ifdef DLMS_IGNORE_MALLOC
        ret = cosem_getDateFromOctectString(value->byteArr, &object->presetTime);
#else
        ret = var_init(&tmp);
        if (ret != 0)
        {
            return ret;
        }
        ret = va_getByIndex(value->Arr, 0, &it);
        if (ret != 0)
        {
            return ret;
        }
        ret = dlms_changeType2(it, DLMS_DATA_TYPE_DATETIME, &tmp);
        if (ret != 0)
        {
            return ret;
        }
        object->presetTime = *tmp.dateTime;
#endif //DLMS_IGNORE_MALLOC
    }
    // Shifts the time.
    else if (index == 6)
    {
        time_addTime(&object->time, 0, 0, var_toInteger(value));
    }
    else
    {
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}
#endif //DLMS_IGNORE_CLOCK
#ifndef DLMS_IGNORE_REGISTER
int invoke_Register(
    gxRegister* object,
    gxValueEventArg* e)
{
    int ret = 0;
    //Reset.
    if (e->index == 1)
    {
        ret = var_clear(&object->value);
    }
    else
    {
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    bb_clear(e->value.byteArr);
    return ret;
}
#endif //DLMS_IGNORE_REGISTER

#ifndef DLMS_IGNORE_PROFILE_GENERIC
/*
* Copies the values of the objects to capture into the buffer by reading
* capture objects.
*/
int capture(dlmsSettings* settings,
    gxProfileGeneric* object)
{
#ifdef DLMS_IGNORE_MALLOC
    gxValueEventArg e;
    gxValueEventCollection args;
    ve_init(&e);
    e.action = 1;
    e.target = &object->base;
    e.index = 2;
    gxValueEventArg p[1] = { e };
    vec_attach(&args, p, 1, 1);
    svr_preGet(settings, &args);
    svr_postGet(settings, &args);
    vec_empty(&args);
#else
    int ret, pos;
    gxKey* kv;
    dlmsVARIANT* value;
    variantArray* row;
    gxValueEventArg e;
    gxValueEventCollection args;
    ve_init(&e);
    e.action = 1;
    e.target = &object->base;
    e.index = 2;
    vec_init(&args);
    vec_push(&args, &e);
    svr_preGet(settings, &args);
    if (!e.handled)
    {
        gxValueEventArg e2;
        ve_init(&e2);
        row = (variantArray*)gxmalloc(sizeof(variantArray));
        va_init(row);
        for (pos = 0; pos != object->captureObjects.size; ++pos)
        {
            ret = arr_getByIndex(&object->captureObjects, pos, (void**)&kv);
            if (ret != DLMS_ERROR_CODE_OK)
            {
                return ret;
            }
            e2.target = (gxObject*)kv->key;
            e2.index = ((gxTarget*)kv->value)->attributeIndex;
            if ((ret = cosem_getData(&e2)) != 0)
            {
                return ret;
            }
            value = (dlmsVARIANT*)gxmalloc(sizeof(dlmsVARIANT));
            var_init(value);
            var_copy(value, &e.value);
            va_push(row, value);
        }
        // Remove first item if buffer is full.
        if (object->profileEntries == object->buffer.size)
        {
            variantArray* removedRow;
            arr_removeByIndex(&object->buffer, 0, (void**)&removedRow);
            va_clear(removedRow);
        }
        arr_push(&object->buffer, row);
        object->entriesInUse = object->buffer.size;
    }
    svr_postGet(settings, &args);
    vec_empty(&args);
#endif //DLMS_IGNORE_MALLOC
    return 0;
}

int invoke_ProfileGeneric(
    dlmsServerSettings* settings,
    gxProfileGeneric* object,
    unsigned char index)
{
    int ret = 0;
    //Reset.
    if (index == 1)
    {
#ifndef DLMS_IGNORE_MALLOC
        ret = obj_clearProfileGenericBuffer(&object->buffer);
#endif //DLMS_IGNORE_MALLOC
    }
    //Capture.
    else if (index == 2)
    {
        // Capture.
        ret = capture(&settings->base, object);
    }
    else
    {
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}
#endif //DLMS_IGNORE_PROFILE_GENERIC


#ifndef DLMS_IGNORE_COMPACT_DATA
int compactDataAppend(unsigned char byteArray, dlmsVARIANT* value3, gxByteBuffer* bb)
{
    if (byteArray && value3->vt == DLMS_DATA_TYPE_OCTET_STRING)
    {
        if (bb_size(value3->byteArr) == 1)
        {
            bb_setUInt8(bb, 0);
        }
        else
        {
            bb_set(bb, value3->byteArr->data + 1, value3->byteArr->size - 1);
        }
        return 0;
    }
    int ret;
    unsigned short startPos = (unsigned short)bb->size;
    if ((ret = dlms_setData(bb, value3->vt, value3)) != 0)
    {
        return ret;
    }
    //If data is empty.
    if (bb->size - startPos == 1)
    {
        bb_setUInt8(bb, 0);
    }
    else
    {
        ret = bb_move(bb, startPos + 1, startPos, bb->size - startPos - 1);
    }
    return 0;
}

int compactDataAppendArray(dlmsVARIANT* value, gxByteBuffer* bb, unsigned short dataIndex)
{
    int ret, pos;
    int cnt = value->Arr->size;
    if (dataIndex != 0)
    {
        cnt = dataIndex;
        --dataIndex;
    }
    dlmsVARIANT* value2;
    for (pos = dataIndex; pos != cnt; ++pos)
    {
        if ((ret = va_getByIndex(value->Arr, pos, &value2)) != 0)
        {
            return ret;
        }
        if (value2->vt == DLMS_DATA_TYPE_STRUCTURE)
        {
            dlmsVARIANT* value3;
            int pos1;
            for (pos1 = 0; pos1 != value2->Arr->size; ++pos1)
            {
                if ((ret = va_getByIndex(value2->Arr, pos1, &value3)) != 0 ||
                    (ret = compactDataAppend(0, value3, bb)) != 0)
                {
                    return ret;
                }
            }
        }
        else
        {
            if ((ret = compactDataAppend(0, value2, bb)) != 0)
            {
                return ret;
            }
        }
    }
    return 0;
}

/*
* Copies the values of the objects to capture into the buffer by reading
* capture objects.
*/
int cosem_captureCompactData(
    dlmsSettings* settings,
    gxCompactData* object)
{
    int ret = 0;
    unsigned short pos;
#ifdef DLMS_IGNORE_MALLOC
    gxTarget* kv;
#else
    gxKey* kv;
#endif //DLMS_IGNORE_MALLOC
    gxValueEventArg e;
    gxValueEventCollection args;
    bb_clear(&object->buffer);
    ve_init(&e);
    e.action = 1;
    e.target = &object->base;
    e.index = 2;
#ifdef DLMS_IGNORE_MALLOC
    //Allocate space where captured values are saved before they are added to the buffer.
    // We can't use server buffer because there might be transaction on progress when this is called.
    unsigned char tmp[MAX_CAPTURE_OBJECT_BUFFER_SIZE];
    gxByteBuffer bb;
    bb_attach(&bb, tmp, 0, sizeof(tmp));
    gxValueEventArg p[1] = { e };
    vec_attach(&args, p, 1, 1);
#else
    vec_init(&args);
    vec_push(&args, &e);
#endif //DLMS_IGNORE_MALLOC
    svr_preGet(settings, &args);
    if (!e.handled)
    {
        unsigned short dataIndex;
        for (pos = 0; pos != object->captureObjects.size; ++pos)
        {
#ifdef DLMS_IGNORE_MALLOC
            ret = arr_getByIndex(&object->captureObjects, pos, (void**)&kv, sizeof(gxTarget));
#else
            ret = arr_getByIndex(&object->captureObjects, pos, (void**)&kv);
#endif //DLMS_IGNORE_MALLOC
            if (ret != DLMS_ERROR_CODE_OK)
            {
                bb_clear(&object->buffer);
                break;
            }
#ifdef DLMS_IGNORE_MALLOC
            e.value.byteArr = &bb;
            e.value.vt = DLMS_DATA_TYPE_OCTET_STRING;
            e.target = kv->target;
            e.index = kv->attributeIndex;
            dataIndex = kv->dataIndex;
#else
            e.target = (gxObject*)kv->key;
            e.index = ((gxTarget*)kv->value)->attributeIndex;
            dataIndex = ((gxTarget*)kv->value)->dataIndex;
#endif //DLMS_IGNORE_MALLOC
            if ((ret = cosem_getValue(settings, &e)) != 0)
            {
                bb_clear(&object->buffer);
                break;
            }
            if (e.byteArray && e.value.vt == DLMS_DATA_TYPE_OCTET_STRING)
            {
                gxDataInfo info;
                dlmsVARIANT value;
                di_init(&info);
                var_init(&value);
                if ((ret = dlms_getData(e.value.byteArr, &info, &value)) != 0)
                {
                    var_clear(&value);
                    break;
                }
                if (value.vt == DLMS_DATA_TYPE_STRUCTURE ||
                    value.vt == DLMS_DATA_TYPE_ARRAY)
                {
#ifdef DLMS_ITALIAN_STANDARD
                    //Some meters require that there is a array count in data.
                    if (value.vt == DLMS_DATA_TYPE_ARRAY && object->appendAA)
                    {
                        bb_setUInt8(&object->buffer, (unsigned char)value.Arr->size);
                    }
#endif //DLMS_ITALIAN_STANDARD
                    if ((ret = compactDataAppendArray(&value, &object->buffer, dataIndex)) != 0)
                    {
                        var_clear(&value);
                        break;
                    }
                }
                else
                {
                    if ((ret = compactDataAppend(1, &e.value, &object->buffer)) != 0)
                    {
                        var_clear(&value);
                        break;
                    }
                }
                var_clear(&value);
            }
            else if ((ret = compactDataAppend(0, &e.value, &object->buffer)) != 0)
            {
                break;
            }
            ve_clear(&e);
        }
    }
    svr_postGet(settings, &args);
    if (ret != 0)
    {
        bb_clear(&object->buffer);
    }
    vec_empty(&args);
    return ret;
}

int invoke_CompactData(
    dlmsServerSettings* settings,
    gxCompactData* object,
    unsigned char index)
{
    int ret = 0;
    //Reset.
    if (index == 1)
    {
        ret = bb_clear(&object->buffer);
    }
    //Capture.
    else if (index == 2)
    {
        // Capture.
        ret = cosem_captureCompactData(&settings->base, object);
    }
    else
    {
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}
#endif //DLMS_IGNORE_COMPACT_DATA

int cosem_invoke(
    dlmsServerSettings* settings,
    gxValueEventArg* e)
{
    int ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    //If invoke index is invalid.
    if (e->index < 1 || e->index > obj_methodCount(e->target))
    {
        return DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    switch (e->target->objectType)
    {
#ifndef DLMS_IGNORE_REGISTER
    case DLMS_OBJECT_TYPE_REGISTER:
        ret = invoke_Register(
            (gxRegister*)e->target,
            e);
        break;
#endif //DLMS_IGNORE_REGISTER
#ifndef DLMS_IGNORE_CLOCK
    case DLMS_OBJECT_TYPE_CLOCK:
        ret = invoke_Clock(
            (gxClock*)e->target,
            e->index, &e->parameters);
        break;
#endif //DLMS_IGNORE_CLOCK
#ifndef DLMS_IGNORE_EXTENDED_REGISTER
    case DLMS_OBJECT_TYPE_EXTENDED_REGISTER:
        ret = invoke_ExtendedRegister(
            (gxExtendedRegister*)e->target,
            e->index);
        break;
#endif //DLMS_IGNORE_EXTENDED_REGISTER
#ifndef DLMS_IGNORE_PROFILE_GENERIC
    case DLMS_OBJECT_TYPE_PROFILE_GENERIC:
        ret = invoke_ProfileGeneric(
            settings,
            (gxProfileGeneric*)e->target,
            e->index);
        break;
#endif //DLMS_IGNORE_PROFILE_GENERIC
#ifndef DLMS_IGNORE_SCRIPT_TABLE
    case DLMS_OBJECT_TYPE_SCRIPT_TABLE:
        ret = invoke_ScriptTable(settings, e);
        break;
#endif //DLMS_IGNORE_SCRIPT_TABLE
#ifndef DLMS_IGNORE_ZIG_BEE_NETWORK_CONTROL
    case DLMS_OBJECT_TYPE_ZIG_BEE_NETWORK_CONTROL:
        ret = invoke_zigbeeNetworkControl(
            (gxZigBeeNetworkControl*)e->target,
            e->index,
            &e->parameters);
        break;
#endif //DLMS_IGNORE_ZIG_BEE_NETWORK_CONTROL
#ifndef DLMS_IGNORE_CHARGE
    case DLMS_OBJECT_TYPE_CHARGE:
        ret = invoke_Charge(
            (gxCharge*)e->target,
            e->index,
            &e->parameters);
        break;
#endif //DLMS_IGNORE_CHARGE
#ifndef DLMS_IGNORE_CREDIT
    case DLMS_OBJECT_TYPE_CREDIT:
        ret = invoke_Credit(
            (gxCredit*)e->target,
            e->index,
            &e->parameters);
        break;
#endif //DLMS_IGNORE_CREDIT
#ifndef DLMS_IGNORE_TOKEN_GATEWAY
    case DLMS_OBJECT_TYPE_TOKEN_GATEWAY:
        ret = invoke_gxTokenGateway(
            (gxTokenGateway*)e->target,
            e->index,
            &e->parameters);
        break;
#endif //DLMS_IGNORE_TOKEN_GATEWAY
#ifndef DLMS_IGNORE_ASSOCIATION_SHORT_NAME
    case DLMS_OBJECT_TYPE_ASSOCIATION_SHORT_NAME:
#ifndef DLMS_IGNORE_ASSOCIATION_SHORT_NAME
        ret = invoke_AssociationShortName(settings, e);
#endif //DLMS_IGNORE_ASSOCIATION_SHORT_NAME
        break;
#endif //ASSOCIATION_SHORT_NAME
#ifndef DLMS_IGNORE_ASSOCIATION_LOGICAL_NAME
    case DLMS_OBJECT_TYPE_ASSOCIATION_LOGICAL_NAME:
        ret = invoke_AssociationLogicalName(settings, e);
        break;
#endif //DLMS_IGNORE_ASSOCIATION_LOGICAL_NAME
#ifndef DLMS_IGNORE_IMAGE_TRANSFER
    case DLMS_OBJECT_TYPE_IMAGE_TRANSFER:
        ret = invoke_ImageTransfer((gxImageTransfer*)e->target, e);
        break;
#endif //DLMS_IGNORE_IMAGE_TRANSFER
#ifndef DLMS_IGNORE_SAP_ASSIGNMENT
    case DLMS_OBJECT_TYPE_SAP_ASSIGNMENT:
        ret = invoke_SapAssigment((gxSapAssignment*)e->target, e);
        break;
#endif //DLMS_IGNORE_SAP_ASSIGNMENT
#ifndef DLMS_IGNORE_COMPACT_DATA
    case DLMS_OBJECT_TYPE_COMPACT_DATA:
        ret = invoke_CompactData(settings, (gxCompactData*)e->target, e->index);
        break;
#endif //DLMS_IGNORE_COMPACT_DATA
        //There are no actions on data component.
    default:
        //Unknown type.
        ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    }
    return ret;
}
#endif //DLMS_IGNORE_SERVER

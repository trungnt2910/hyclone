/*
 * Copyright 2005-2010 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *        Erik Jaesler (erik@cgsoftware.com)
 */
#ifndef __HAIKU_TYPE_CONSTANTS_H__
#define __HAIKU_TYPE_CONSTANTS_H__

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmultichar"
#endif

enum
{
    B_AFFINE_TRANSFORM_TYPE = 'AMTX',
    B_ALIGNMENT_TYPE = 'ALGN',
    B_ANY_TYPE = 'ANYT',
    B_ATOM_TYPE = 'ATOM',
    B_ATOMREF_TYPE = 'ATMR',
    B_BOOL_TYPE = 'BOOL',
    B_CHAR_TYPE = 'CHAR',
    B_COLOR_8_BIT_TYPE = 'CLRB',
    B_DOUBLE_TYPE = 'DBLE',
    B_FLOAT_TYPE = 'FLOT',
    B_GRAYSCALE_8_BIT_TYPE = 'GRYB',
    B_INT16_TYPE = 'SHRT',
    B_INT32_TYPE = 'LONG',
    B_INT64_TYPE = 'LLNG',
    B_INT8_TYPE = 'BYTE',
    B_LARGE_ICON_TYPE = 'ICON',
    B_MEDIA_PARAMETER_GROUP_TYPE = 'BMCG',
    B_MEDIA_PARAMETER_TYPE = 'BMCT',
    B_MEDIA_PARAMETER_WEB_TYPE = 'BMCW',
    B_MESSAGE_TYPE = 'MSGG',
    B_MESSENGER_TYPE = 'MSNG',
    B_MIME_TYPE = 'MIME',
    B_MINI_ICON_TYPE = 'MICN',
    B_MONOCHROME_1_BIT_TYPE = 'MNOB',
    B_OBJECT_TYPE = 'OPTR',
    B_OFF_T_TYPE = 'OFFT',
    B_PATTERN_TYPE = 'PATN',
    B_POINTER_TYPE = 'PNTR',
    B_POINT_TYPE = 'BPNT',
    B_PROPERTY_INFO_TYPE = 'SCTD',
    B_RAW_TYPE = 'RAWT',
    B_RECT_TYPE = 'RECT',
    B_REF_TYPE = 'RREF',
    B_NODE_REF_TYPE = 'NREF',
    B_RGB_32_BIT_TYPE = 'RGBB',
    B_RGB_COLOR_TYPE = 'RGBC',
    B_SIZE_TYPE = 'SIZE',
    B_SIZE_T_TYPE = 'SIZT',
    B_SSIZE_T_TYPE = 'SSZT',
    B_STRING_TYPE = 'CSTR',
    B_STRING_LIST_TYPE = 'STRL',
    B_TIME_TYPE = 'TIME',
    B_UINT16_TYPE = 'USHT',
    B_UINT32_TYPE = 'ULNG',
    B_UINT64_TYPE = 'ULLG',
    B_UINT8_TYPE = 'UBYT',
    B_VECTOR_ICON_TYPE = 'VICN',
    B_XATTR_TYPE = 'XATR',
    B_NETWORK_ADDRESS_TYPE = 'NWAD',
    B_MIME_STRING_TYPE = 'MIMS',

    /* deprecated, do not use */
    B_ASCII_TYPE = 'TEXT'
    /* use B_STRING_TYPE instead */
};

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#endif /* __HAIKU_TYPE_CONSTANTS_H__ */
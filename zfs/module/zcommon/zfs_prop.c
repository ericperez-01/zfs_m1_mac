/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2011, 2016 by Delphix. All rights reserved.
 * Copyright (c) 2013 by Saso Kiselkov. All rights reserved.
 */

/* Portions Copyright 2010 Robert Milkowski */

#include <sys/zio.h>
#include <sys/spa.h>
#include <sys/u8_textprep.h>
#include <sys/zfs_acl.h>
#include <sys/zfs_ioctl.h>
#include <sys/zfs_znode.h>
#include <sys/dsl_crypt.h>

#include "zfs_prop.h"
#include "zfs_deleg.h"
#include "zfs_fletcher.h"

#if defined(_KERNEL)
#include <sys/systm.h>
#else
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#endif

static zprop_desc_t zfs_prop_table[ZFS_NUM_PROPS];

/* Note this is indexed by zfs_userquota_prop_t, keep the order the same */
const char *zfs_userquota_prop_prefixes[] = {
	"userused@",
	"userquota@",
	"groupused@",
	"groupquota@"
};

zprop_desc_t *
zfs_prop_get_table(void)
{
	return (zfs_prop_table);
}

void
zfs_prop_init(void)
{
	static zprop_index_t checksum_table[] = {
		{ "on",			ZIO_CHECKSUM_ON },
		{ "off",		ZIO_CHECKSUM_OFF },
		{ "fletcher2",	ZIO_CHECKSUM_FLETCHER_2 },
		{ "fletcher4",	ZIO_CHECKSUM_FLETCHER_4 },
		{ "sha256",		ZIO_CHECKSUM_SHA256 },
		{ "noparity",   ZIO_CHECKSUM_NOPARITY },
		{ "sha512",     ZIO_CHECKSUM_SHA512 },
		{ "skein",      ZIO_CHECKSUM_SKEIN },
		{ "edonr",      ZIO_CHECKSUM_EDONR },
		{ NULL }
	};

	static zprop_index_t dedup_table[] = {
		{ "on",		ZIO_CHECKSUM_ON },
		{ "off",	ZIO_CHECKSUM_OFF },
		{ "verify",	ZIO_CHECKSUM_ON | ZIO_CHECKSUM_VERIFY },
		{ "sha256",	ZIO_CHECKSUM_SHA256 },
		{ "sha256,verify",
		  ZIO_CHECKSUM_SHA256 | ZIO_CHECKSUM_VERIFY },
		{ "sha512",     ZIO_CHECKSUM_SHA512 },
		{ "sha512,verify",
		  ZIO_CHECKSUM_SHA512 | ZIO_CHECKSUM_VERIFY },
		{ "skein",      ZIO_CHECKSUM_SKEIN },
		{ "skein,verify",
		  ZIO_CHECKSUM_SKEIN | ZIO_CHECKSUM_VERIFY },
		{ "edonr,verify",
		  ZIO_CHECKSUM_EDONR | ZIO_CHECKSUM_VERIFY },
		{ NULL }
	};

	static zprop_index_t compress_table[] = {
		{ "on",		ZIO_COMPRESS_ON },
		{ "off",	ZIO_COMPRESS_OFF },
		{ "lzjb",	ZIO_COMPRESS_LZJB },
		{ "gzip",	ZIO_COMPRESS_GZIP_6 },	/* gzip default */
		{ "gzip-1",	ZIO_COMPRESS_GZIP_1 },
		{ "gzip-2",	ZIO_COMPRESS_GZIP_2 },
		{ "gzip-3",	ZIO_COMPRESS_GZIP_3 },
		{ "gzip-4",	ZIO_COMPRESS_GZIP_4 },
		{ "gzip-5",	ZIO_COMPRESS_GZIP_5 },
		{ "gzip-6",	ZIO_COMPRESS_GZIP_6 },
		{ "gzip-7",	ZIO_COMPRESS_GZIP_7 },
		{ "gzip-8",	ZIO_COMPRESS_GZIP_8 },
		{ "gzip-9",	ZIO_COMPRESS_GZIP_9 },
		{ "zle",	ZIO_COMPRESS_ZLE },
		{ "lz4",	ZIO_COMPRESS_LZ4 },
		{ NULL }
	};

	static zprop_index_t crypto_table[] = {
		{ "on",			ZIO_CRYPT_ON },
		{ "off",		ZIO_CRYPT_OFF },
		{ "aes-128-ccm",	ZIO_CRYPT_AES_128_CCM },
		{ "aes-192-ccm",	ZIO_CRYPT_AES_192_CCM },
		{ "aes-256-ccm",	ZIO_CRYPT_AES_256_CCM },
		{ "aes-128-gcm",	ZIO_CRYPT_AES_128_GCM },
		{ "aes-192-gcm",	ZIO_CRYPT_AES_192_GCM },
		{ "aes-256-gcm",	ZIO_CRYPT_AES_256_GCM },
		{ NULL }
	};

	static zprop_index_t keyformat_table[] = {
		{ "none",		ZFS_KEYFORMAT_NONE },
		{ "raw",		ZFS_KEYFORMAT_RAW },
		{ "hex",		ZFS_KEYFORMAT_HEX },
		{ "passphrase",		ZFS_KEYFORMAT_PASSPHRASE },
		{ NULL }
	};

	static zprop_index_t snapdir_table[] = {
		{ "hidden",	ZFS_SNAPDIR_HIDDEN },
		{ "visible",	ZFS_SNAPDIR_VISIBLE },
		{ NULL }
	};

	static zprop_index_t snapdev_table[] = {
		{ "hidden",	ZFS_SNAPDEV_HIDDEN },
		{ "visible",	ZFS_SNAPDEV_VISIBLE },
		{ NULL }
	};

#ifdef __LINUX__
	static zprop_index_t acltype_table[] = {
		{ "off",	ZFS_ACLTYPE_OFF },
		{ "disabled",	ZFS_ACLTYPE_OFF },
		{ "noacl",	ZFS_ACLTYPE_OFF },
		{ "posixacl",	ZFS_ACLTYPE_POSIXACL },
		{ NULL }
	};
#endif

	static zprop_index_t acl_inherit_table[] = {
		{ "discard",	ZFS_ACL_DISCARD },
		{ "noallow",	ZFS_ACL_NOALLOW },
		{ "restricted",	ZFS_ACL_RESTRICTED },
		{ "passthrough", ZFS_ACL_PASSTHROUGH },
		{ "secure",	ZFS_ACL_RESTRICTED }, /* bkwrd compatability */
		{ "passthrough-x", ZFS_ACL_PASSTHROUGH_X },
		{ NULL }
	};

    static zprop_index_t acl_mode_table[] = {
        { "discard",    ZFS_ACL_DISCARD },
        { "discard",    ZFS_ACL_GROUPMASK }, /* bkwrd compatability */
        { "passthrough", ZFS_ACL_PASSTHROUGH },
        { "mask", ZFS_ACL_MASKED },
        { NULL }
    };

	static zprop_index_t case_table[] = {
		{ "sensitive",		ZFS_CASE_SENSITIVE },
		{ "insensitive",	ZFS_CASE_INSENSITIVE },
		{ "mixed",		ZFS_CASE_MIXED },
		{ NULL }
	};

	static zprop_index_t copies_table[] = {
		{ "1",		1 },
		{ "2",		2 },
		{ "3",		3 },
		{ NULL }
	};

	/*
	 * Use the unique flags we have to send to u8_strcmp() and/or
	 * u8_textprep() to represent the various normalization property
	 * values.
	 */
	static zprop_index_t normalize_table[] = {
		{ "none",	0 },
		{ "formD",	U8_TEXTPREP_NFD },
		{ "formKC",	U8_TEXTPREP_NFKC },
		{ "formC",	U8_TEXTPREP_NFC },
		{ "formKD",	U8_TEXTPREP_NFKD },
		{ NULL }
	};

	static zprop_index_t version_table[] = {
		{ "1",		1 },
		{ "2",		2 },
		{ "3",		3 },
		{ "4",		4 },
		{ "5",		5 },
		{ "current",	ZPL_VERSION },
		{ NULL }
	};

	static zprop_index_t boolean_table[] = {
		{ "off",	0 },
		{ "on",		1 },
		{ NULL }
	};

	static zprop_index_t keystatus_table[] = {
		{ "none",		ZFS_KEYSTATUS_NONE},
		{ "unavailable",	ZFS_KEYSTATUS_UNAVAILABLE},
		{ "available",		ZFS_KEYSTATUS_AVAILABLE},
		{ NULL }
	};

	static zprop_index_t logbias_table[] = {
		{ "latency",	ZFS_LOGBIAS_LATENCY },
		{ "throughput",	ZFS_LOGBIAS_THROUGHPUT },
		{ NULL }
	};

	static zprop_index_t canmount_table[] = {
		{ "off",	ZFS_CANMOUNT_OFF },
		{ "on",		ZFS_CANMOUNT_ON },
		{ "noauto",	ZFS_CANMOUNT_NOAUTO },
		{ NULL }
	};

	static zprop_index_t cache_table[] = {
		{ "none",	ZFS_CACHE_NONE },
		{ "metadata",	ZFS_CACHE_METADATA },
		{ "all",	ZFS_CACHE_ALL },
		{ NULL }
	};

	static zprop_index_t sync_table[] = {
		{ "standard",	ZFS_SYNC_STANDARD },
		{ "always",	ZFS_SYNC_ALWAYS },
		{ "disabled",	ZFS_SYNC_DISABLED },
		{ NULL }
	};

	static zprop_index_t xattr_table[] = {
		{ "off",	ZFS_XATTR_OFF },
		{ "on",		ZFS_XATTR_DIR },
		{ "sa",		ZFS_XATTR_SA },
		{ "dir",	ZFS_XATTR_DIR },
		{ NULL }
	};

	static zprop_index_t dnsize_table[] = {
		{ "legacy",	ZFS_DNSIZE_LEGACY },
		{ "auto",	ZFS_DNSIZE_AUTO },
		{ "1k",		ZFS_DNSIZE_1K },
		{ "2k",		ZFS_DNSIZE_2K },
		{ "4k",		ZFS_DNSIZE_4K },
		{ "8k",		ZFS_DNSIZE_8K },
		{ "16k",	ZFS_DNSIZE_16K },
		{ NULL }
	};

	static zprop_index_t redundant_metadata_table[] = {
		{ "all",	ZFS_REDUNDANT_METADATA_ALL },
		{ "most",	ZFS_REDUNDANT_METADATA_MOST },
		{ NULL }
	};

#ifdef __APPLE__
	static zprop_index_t devdisk_table[] = {
		{ "poolonly",	ZFS_DEVDISK_POOLONLY },
		{ "off",		ZFS_DEVDISK_OFF },
		{ "on",			ZFS_DEVDISK_ON },
		{ NULL }
	};
#endif

	/* inherit index properties */
	zprop_register_index(ZFS_PROP_REDUNDANT_METADATA, "redundant_metadata",
	    ZFS_REDUNDANT_METADATA_ALL,
	    PROP_INHERIT, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
	    "all | most", "REDUND_MD",
	    redundant_metadata_table);
	zprop_register_index(ZFS_PROP_SYNC, "sync", ZFS_SYNC_STANDARD,
	    PROP_INHERIT, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
	    "standard | always | disabled", "SYNC",
	    sync_table);
	zprop_register_index(ZFS_PROP_CHECKSUM, "checksum",
	    ZIO_CHECKSUM_DEFAULT, PROP_INHERIT, ZFS_TYPE_FILESYSTEM |
	    ZFS_TYPE_VOLUME,
		"on | off | fletcher2 | fletcher4 | sha256 | sha512 | "
		"skein | edonr", "CHECKSUM", checksum_table);
	zprop_register_index(ZFS_PROP_DEDUP, "dedup", ZIO_CHECKSUM_OFF,
	    PROP_INHERIT, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
		"on | off | verify | sha256[,verify], sha512[,verify], "
		"skein[,verify], edonr,verify", "DEDUP", dedup_table);
	zprop_register_index(ZFS_PROP_COMPRESSION, "compression",
	    ZIO_COMPRESS_DEFAULT, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
	    "on | off | lzjb | gzip | gzip-[1-9] | zle | lz4", "COMPRESS",
	    compress_table);
	zprop_register_index(ZFS_PROP_SNAPDIR, "snapdir", ZFS_SNAPDIR_HIDDEN,
	    PROP_INHERIT, ZFS_TYPE_FILESYSTEM,
	    "hidden | visible", "SNAPDIR", snapdir_table);
	zprop_register_index(ZFS_PROP_SNAPDEV, "snapdev", ZFS_SNAPDEV_HIDDEN,
	    PROP_INHERIT, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
	    "hidden | visible", "SNAPDEV", snapdev_table);
#ifdef LINUX
	zprop_register_index(ZFS_PROP_ACLTYPE, "acltype", ZFS_ACLTYPE_OFF,
	    PROP_INHERIT, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_SNAPSHOT,
	    "noacl | posixacl", "ACLTYPE", acltype_table);
#endif
	zprop_register_index(ZFS_PROP_ACLINHERIT, "aclinherit",
	    ZFS_ACL_RESTRICTED, PROP_INHERIT, ZFS_TYPE_FILESYSTEM,
	    "discard | noallow | restricted | passthrough | passthrough-x",
	    "ACLINHERIT", acl_inherit_table);
    zprop_register_index(ZFS_PROP_ACLMODE, "aclmode",
        ZFS_ACL_PASSTHROUGH, PROP_INHERIT, ZFS_TYPE_FILESYSTEM,
        "discard | mask | passthrough", "ACLMODE", acl_mode_table);
	zprop_register_index(ZFS_PROP_COPIES, "copies", 1, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
	    "1 | 2 | 3", "COPIES", copies_table);
	zprop_register_index(ZFS_PROP_PRIMARYCACHE, "primarycache",
	    ZFS_CACHE_ALL, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_SNAPSHOT | ZFS_TYPE_VOLUME,
	    "all | none | metadata", "PRIMARYCACHE", cache_table);
	zprop_register_index(ZFS_PROP_SECONDARYCACHE, "secondarycache",
	    ZFS_CACHE_ALL, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_SNAPSHOT | ZFS_TYPE_VOLUME,
	    "all | none | metadata", "SECONDARYCACHE", cache_table);
	zprop_register_index(ZFS_PROP_LOGBIAS, "logbias", ZFS_LOGBIAS_LATENCY,
	    PROP_INHERIT, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
	    "latency | throughput", "LOGBIAS", logbias_table);
	zprop_register_index(ZFS_PROP_XATTR, "xattr", ZFS_XATTR_DIR,
	    PROP_INHERIT, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_SNAPSHOT,
	    "on | off | dir | sa", "XATTR", xattr_table);

	zprop_register_index(ZFS_PROP_DNODESIZE, "dnodesize",
	    ZFS_DNSIZE_LEGACY, PROP_INHERIT, ZFS_TYPE_FILESYSTEM,
	    "legacy | auto | 1k | 2k | 4k | 8k | 16k", "DNSIZE", dnsize_table);

	/* inherit index (boolean) properties */
	zprop_register_index(ZFS_PROP_ATIME, "atime", 1, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM, "on | off", "ATIME", boolean_table);
#ifdef LINUX
	zprop_register_index(ZFS_PROP_RELATIME, "relatime", 0, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM, "on | off", "RELATIME", boolean_table);
#endif
	zprop_register_index(ZFS_PROP_DEVICES, "devices", 1, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_SNAPSHOT, "on | off", "DEVICES",
	    boolean_table);
	zprop_register_index(ZFS_PROP_EXEC, "exec", 1, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_SNAPSHOT, "on | off", "EXEC",
	    boolean_table);
	zprop_register_index(ZFS_PROP_SETUID, "setuid", 1, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_SNAPSHOT, "on | off", "SETUID",
	    boolean_table);
	zprop_register_index(ZFS_PROP_READONLY, "readonly", 0, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME, "on | off", "RDONLY",
	    boolean_table);
	zprop_register_index(ZFS_PROP_ZONED, "zoned", 0, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM, "on | off", "ZONED", boolean_table);
	zprop_register_index(ZFS_PROP_VSCAN, "vscan", 0, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM, "on | off", "VSCAN", boolean_table);
	zprop_register_index(ZFS_PROP_NBMAND, "nbmand", 0, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_SNAPSHOT, "on | off", "NBMAND",
	    boolean_table);
	zprop_register_index(ZFS_PROP_OVERLAY, "overlay", 0, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM, "on | off", "OVERLAY", boolean_table);

	/* default index properties */
	zprop_register_index(ZFS_PROP_VERSION, "version", 0, PROP_DEFAULT,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_SNAPSHOT,
	    "1 | 2 | 3 | 4 | 5 | current", "VERSION", version_table);
	zprop_register_index(ZFS_PROP_CANMOUNT, "canmount", ZFS_CANMOUNT_ON,
	    PROP_DEFAULT, ZFS_TYPE_FILESYSTEM, "on | off | noauto",
	    "CANMOUNT", canmount_table);

	/* readonly index properties */
	zprop_register_index(ZFS_PROP_MOUNTED, "mounted", 0, PROP_READONLY,
#ifdef __APPLE__
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_SNAPSHOT, "yes | no", "MOUNTED", boolean_table);
#else
	    ZFS_TYPE_FILESYSTEM, "yes | no", "MOUNTED", boolean_table);
#endif
	zprop_register_index(ZFS_PROP_DEFER_DESTROY, "defer_destroy", 0,
	    PROP_READONLY, ZFS_TYPE_SNAPSHOT, "yes | no", "DEFER_DESTROY",
	    boolean_table);
	zprop_register_index(ZFS_PROP_KEYSTATUS, "keystatus",
		ZFS_KEYSTATUS_NONE, PROP_READONLY, ZFS_TYPE_DATASET,
		"none | unavailable | available",
		"KEYSTATUS", keystatus_table);

	/* set once index properties */
	zprop_register_index(ZFS_PROP_NORMALIZE, "normalization", 0,
	    PROP_ONETIME, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_SNAPSHOT,
	    "none | formC | formD | formKC | formKD", "NORMALIZATION",
	    normalize_table);
	zprop_register_index(ZFS_PROP_CASE, "casesensitivity",
	    ZFS_CASE_SENSITIVE, PROP_ONETIME, ZFS_TYPE_FILESYSTEM |
	    ZFS_TYPE_SNAPSHOT,
	    "sensitive | insensitive | mixed", "CASE", case_table);
	zprop_register_index(ZFS_PROP_KEYFORMAT, "keyformat",
	    ZFS_KEYFORMAT_NONE, PROP_ONETIME_DEFAULT,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
	    "none | raw | hex | passphrase", "KEYFORMAT", keyformat_table);
	zprop_register_index(ZFS_PROP_ENCRYPTION, "encryption",
	    ZIO_CRYPT_DEFAULT, PROP_ONETIME, ZFS_TYPE_DATASET,
	    "on | off | aes-128-ccm | aes-192-ccm | aes-256-ccm | "
	    "aes-128-gcm | aes-192-gcm | aes-256-gcm", "ENCRYPTION",
	    crypto_table);

	/* set once index (boolean) properties */
	zprop_register_index(ZFS_PROP_UTF8ONLY, "utf8only", 0, PROP_ONETIME,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_SNAPSHOT,
	    "on | off", "UTF8ONLY", boolean_table);

	/* string properties */
	zprop_register_string(ZFS_PROP_ORIGIN, "origin", NULL, PROP_READONLY,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME, "<snapshot>", "ORIGIN");
	zprop_register_string(ZFS_PROP_CLONES, "clones", NULL, PROP_READONLY,
	    ZFS_TYPE_SNAPSHOT, "<dataset>[,...]", "CLONES");
	zprop_register_string(ZFS_PROP_MOUNTPOINT, "mountpoint", "/",
#ifdef __APPLE__
	    PROP_INHERIT, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_SNAPSHOT, "<path> | legacy | none",
#else
	    PROP_INHERIT, ZFS_TYPE_FILESYSTEM, "<path> | legacy | none",
#endif
	    "MOUNTPOINT");
	zprop_register_string(ZFS_PROP_SHARENFS, "sharenfs", "off",
	    PROP_INHERIT, ZFS_TYPE_FILESYSTEM, "on | off | share(1M) options",
	    "SHARENFS");
	zprop_register_string(ZFS_PROP_TYPE, "type", NULL, PROP_READONLY,
	    ZFS_TYPE_DATASET | ZFS_TYPE_BOOKMARK,
	    "filesystem | volume | snapshot | bookmark", "TYPE");
	zprop_register_string(ZFS_PROP_SHARESMB, "sharesmb", "off",
	    PROP_INHERIT, ZFS_TYPE_FILESYSTEM,
	    "on | off | sharemgr(1M) options", "SHARESMB");
#ifdef __APPLE__
    zprop_register_string(ZFS_PROP_SHAREAFP, "shareafp", "off",
	    PROP_INHERIT, ZFS_TYPE_FILESYSTEM,
	    "on | off | sharing(1) options", "SHAREAFP");
#endif
    zprop_register_string(ZFS_PROP_MLSLABEL, "mlslabel",
	    ZFS_MLSLABEL_DEFAULT, PROP_INHERIT, ZFS_TYPE_DATASET,
	    "<sensitivity label>", "MLSLABEL");
    zprop_register_string(ZFS_PROP_RECEIVE_RESUME_TOKEN,
		"receive_resume_token",
		NULL, PROP_READONLY, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
		"<string token>", "RESUMETOK");

#ifdef LINUX
	zprop_register_string(ZFS_PROP_SELINUX_CONTEXT, "context",
	    "none", PROP_DEFAULT, ZFS_TYPE_DATASET, "<selinux context>",
	    "CONTEXT");
	zprop_register_string(ZFS_PROP_SELINUX_FSCONTEXT, "fscontext",
	    "none", PROP_DEFAULT, ZFS_TYPE_DATASET, "<selinux fscontext>",
	    "FSCONTEXT");
	zprop_register_string(ZFS_PROP_SELINUX_DEFCONTEXT, "defcontext",
	    "none", PROP_DEFAULT, ZFS_TYPE_DATASET, "<selinux defcontext>",
	    "DEFCONTEXT");
	zprop_register_string(ZFS_PROP_SELINUX_ROOTCONTEXT, "rootcontext",
	    "none", PROP_DEFAULT, ZFS_TYPE_DATASET, "<selinux rootcontext>",
	    "ROOTCONTEXT");
#endif
	zprop_register_string(ZFS_PROP_RECEIVE_RESUME_TOKEN,
	    "receive_resume_token",
	    NULL, PROP_READONLY, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
	    "<string token>", "RESUMETOK");
	zprop_register_string(ZFS_PROP_ENCRYPTION_ROOT, "encryptionroot", NULL,
	    PROP_READONLY, ZFS_TYPE_DATASET, "<filesystem | volume>",
	    "ENCROOT");
	zprop_register_string(ZFS_PROP_KEYLOCATION, "keylocation",
	    "none", PROP_DEFAULT, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
	    "prompt | <file URI>", "KEYLOCATION");

	/* readonly number properties */
	zprop_register_number(ZFS_PROP_USED, "used", 0, PROP_READONLY,
	    ZFS_TYPE_DATASET, "<size>", "USED");
	zprop_register_number(ZFS_PROP_AVAILABLE, "available", 0, PROP_READONLY,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME, "<size>", "AVAIL");
	zprop_register_number(ZFS_PROP_REFERENCED, "referenced", 0,
	    PROP_READONLY, ZFS_TYPE_DATASET, "<size>", "REFER");
	zprop_register_number(ZFS_PROP_COMPRESSRATIO, "compressratio", 0,
	    PROP_READONLY, ZFS_TYPE_DATASET,
	    "<1.00x or higher if compressed>", "RATIO");
	zprop_register_number(ZFS_PROP_REFRATIO, "refcompressratio", 0,
	    PROP_READONLY, ZFS_TYPE_DATASET,
	    "<1.00x or higher if compressed>", "REFRATIO");
	zprop_register_number(ZFS_PROP_VOLBLOCKSIZE, "volblocksize",
	    ZVOL_DEFAULT_BLOCKSIZE, PROP_ONETIME,
	    ZFS_TYPE_VOLUME, "512 to 128k, power of 2",	"VOLBLOCK");
	zprop_register_number(ZFS_PROP_USEDSNAP, "usedbysnapshots", 0,
	    PROP_READONLY, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME, "<size>",
	    "USEDSNAP");
	zprop_register_number(ZFS_PROP_USEDDS, "usedbydataset", 0,
	    PROP_READONLY, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME, "<size>",
	    "USEDDS");
	zprop_register_number(ZFS_PROP_USEDCHILD, "usedbychildren", 0,
	    PROP_READONLY, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME, "<size>",
	    "USEDCHILD");
	zprop_register_number(ZFS_PROP_USEDREFRESERV, "usedbyrefreservation", 0,
	    PROP_READONLY,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME, "<size>", "USEDREFRESERV");
	zprop_register_number(ZFS_PROP_USERREFS, "userrefs", 0, PROP_READONLY,
	    ZFS_TYPE_SNAPSHOT, "<count>", "USERREFS");
	zprop_register_number(ZFS_PROP_WRITTEN, "written", 0, PROP_READONLY,
	    ZFS_TYPE_DATASET, "<size>", "WRITTEN");
	zprop_register_number(ZFS_PROP_LOGICALUSED, "logicalused", 0,
	    PROP_READONLY, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME, "<size>",
	    "LUSED");
	zprop_register_number(ZFS_PROP_LOGICALREFERENCED, "logicalreferenced",
	    0, PROP_READONLY, ZFS_TYPE_DATASET, "<size>", "LREFER");
	zprop_register_number(ZFS_PROP_PBKDF2_ITERS, "pbkdf2iters",
	    0, PROP_ONETIME_DEFAULT, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
	    "<iters>", "PBKDF2ITERS");

	/* default number properties */
	zprop_register_number(ZFS_PROP_QUOTA, "quota", 0, PROP_DEFAULT,
	    ZFS_TYPE_FILESYSTEM, "<size> | none", "QUOTA");
	zprop_register_number(ZFS_PROP_RESERVATION, "reservation", 0,
	    PROP_DEFAULT, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
	    "<size> | none", "RESERV");
	zprop_register_number(ZFS_PROP_VOLSIZE, "volsize", 0, PROP_DEFAULT,
	    ZFS_TYPE_SNAPSHOT | ZFS_TYPE_VOLUME, "<size>", "VOLSIZE");
	zprop_register_number(ZFS_PROP_REFQUOTA, "refquota", 0, PROP_DEFAULT,
	    ZFS_TYPE_FILESYSTEM, "<size> | none", "REFQUOTA");
	zprop_register_number(ZFS_PROP_REFRESERVATION, "refreservation", 0,
	    PROP_DEFAULT, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
	    "<size> | none", "REFRESERV");
	zprop_register_number(ZFS_PROP_FILESYSTEM_LIMIT, "filesystem_limit",
	    UINT64_MAX, PROP_DEFAULT, ZFS_TYPE_FILESYSTEM,
	    "<count> | none", "FSLIMIT");
	zprop_register_number(ZFS_PROP_SNAPSHOT_LIMIT, "snapshot_limit",
	    UINT64_MAX, PROP_DEFAULT, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
	    "<count> | none", "SSLIMIT");
	zprop_register_number(ZFS_PROP_FILESYSTEM_COUNT, "filesystem_count",
	    UINT64_MAX, PROP_DEFAULT, ZFS_TYPE_FILESYSTEM,
	    "<count>", "FSCOUNT");
	zprop_register_number(ZFS_PROP_SNAPSHOT_COUNT, "snapshot_count",
	    UINT64_MAX, PROP_DEFAULT, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME,
	    "<count>", "SSCOUNT");

	/* inherit number properties */
	zprop_register_number(ZFS_PROP_RECORDSIZE, "recordsize",
	    SPA_OLD_MAXBLOCKSIZE, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM, "512 to 1M, power of 2", "RECSIZE");
	zprop_register_number(ZFS_PROP_SPECIAL_SMALL_BLOCKS,
	    "special_small_blocks", 0, PROP_INHERIT, ZFS_TYPE_FILESYSTEM,
	    "zero or 512 to 128K, power of 2", "SPECIAL_SMALL_BLOCKS");

	/* hidden properties */
	zprop_register_hidden(ZFS_PROP_CREATETXG, "createtxg", PROP_TYPE_NUMBER,
	    PROP_READONLY, ZFS_TYPE_DATASET | ZFS_TYPE_BOOKMARK, "CREATETXG");
	zprop_register_hidden(ZFS_PROP_REMAPTXG, "remaptxg", PROP_TYPE_NUMBER,
	    PROP_READONLY, ZFS_TYPE_DATASET, "REMAPTXG");
	zprop_register_hidden(ZFS_PROP_NUMCLONES, "numclones", PROP_TYPE_NUMBER,
	    PROP_READONLY, ZFS_TYPE_SNAPSHOT, "NUMCLONES");
	zprop_register_hidden(ZFS_PROP_NAME, "name", PROP_TYPE_STRING,
	    PROP_READONLY, ZFS_TYPE_DATASET | ZFS_TYPE_BOOKMARK, "NAME");
	zprop_register_hidden(ZFS_PROP_ISCSIOPTIONS, "iscsioptions",
	    PROP_TYPE_STRING, PROP_INHERIT, ZFS_TYPE_VOLUME, "ISCSIOPTIONS");
	zprop_register_hidden(ZFS_PROP_STMF_SHAREINFO, "stmf_sbd_lu",
	    PROP_TYPE_STRING, PROP_INHERIT, ZFS_TYPE_VOLUME,
	    "STMF_SBD_LU");
	zprop_register_hidden(ZFS_PROP_GUID, "guid", PROP_TYPE_NUMBER,
	    PROP_READONLY, ZFS_TYPE_DATASET | ZFS_TYPE_BOOKMARK, "GUID");
	zprop_register_hidden(ZFS_PROP_USERACCOUNTING, "useraccounting",
	    PROP_TYPE_NUMBER, PROP_READONLY, ZFS_TYPE_DATASET,
	    "USERACCOUNTING");
	zprop_register_hidden(ZFS_PROP_UNIQUE, "unique", PROP_TYPE_NUMBER,
	    PROP_READONLY, ZFS_TYPE_DATASET, "UNIQUE");
	zprop_register_hidden(ZFS_PROP_OBJSETID, "objsetid", PROP_TYPE_NUMBER,
	    PROP_READONLY, ZFS_TYPE_DATASET, "OBJSETID");
	zprop_register_hidden(ZFS_PROP_INCONSISTENT, "inconsistent",
	    PROP_TYPE_NUMBER, PROP_READONLY, ZFS_TYPE_DATASET, "INCONSISTENT");
	zprop_register_hidden(ZFS_PROP_IVSET_GUID, "ivsetguid",
	    PROP_TYPE_NUMBER, PROP_READONLY,
	    ZFS_TYPE_DATASET | ZFS_TYPE_BOOKMARK, "IVSETGUID");
	zprop_register_hidden(ZFS_PROP_PREV_SNAP, "prevsnap", PROP_TYPE_STRING,
	    PROP_READONLY, ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME, "PREVSNAP");
	zprop_register_hidden(ZFS_PROP_PBKDF2_SALT, "pbkdf2salt",
	    PROP_TYPE_NUMBER, PROP_ONETIME_DEFAULT,
	    ZFS_TYPE_FILESYSTEM | ZFS_TYPE_VOLUME, "PBKDF2SALT");
	zprop_register_hidden(ZFS_PROP_KEY_GUID, "keyguid", PROP_TYPE_NUMBER,
	    PROP_READONLY, ZFS_TYPE_DATASET, "KEYGUID");

	/* oddball properties */
	zprop_register_impl(ZFS_PROP_CREATION, "creation", PROP_TYPE_NUMBER, 0,
	    NULL, PROP_READONLY, ZFS_TYPE_DATASET | ZFS_TYPE_BOOKMARK,
	    "<date>", "CREATION", B_FALSE, B_TRUE, NULL);

#ifdef __APPLE__
    /* Apple specific properties */
	zprop_register_index(ZFS_PROP_APPLE_BROWSE, "com.apple.browse", 1,PROP_INHERIT,
        ZFS_TYPE_FILESYSTEM, "on | off", "COM.APPLE.BROWSE", boolean_table);
	zprop_register_index(ZFS_PROP_APPLE_IGNOREOWNER, "com.apple.ignoreowner", 0, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM, "on | off", "COM.APPLE.IGNOREOWNER", boolean_table);
	zprop_register_hidden(ZFS_PROP_APPLE_LASTUNMOUNT,"COM.APPLE.LASTUNMOUNT",
	    PROP_TYPE_NUMBER, PROP_READONLY, ZFS_TYPE_DATASET, "LASTUNMOUNT");
	zprop_register_index(ZFS_PROP_APPLE_MIMIC_HFS, "com.apple.mimic_hfs", 0, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM, "on | off", "COM.APPLE.MIMIC_HFS", boolean_table);
	zprop_register_index(ZFS_PROP_APPLE_DEVDISK, "com.apple.devdisk", 0, PROP_INHERIT,
	    ZFS_TYPE_FILESYSTEM, "poolonly | on | off", "COM.APPLE.DEVDISK", devdisk_table);
#endif


}

boolean_t
zfs_prop_delegatable(zfs_prop_t prop)
{
	zprop_desc_t *pd = &zfs_prop_table[prop];

	/* The mlslabel property is never delegatable. */
	if (prop == ZFS_PROP_MLSLABEL)
		return (B_FALSE);

	return (pd->pd_attr != PROP_READONLY);
}

/*
 * Given a zfs dataset property name, returns the corresponding property ID.
 */
zfs_prop_t
zfs_name_to_prop(const char *propname)
{
	return (zprop_name_to_prop(propname, ZFS_TYPE_DATASET));
}

/*
 * For user property names, we allow all lowercase alphanumeric characters, plus
 * a few useful punctuation characters.
 */
static int
valid_char(char c)
{
	return ((c >= 'a' && c <= 'z') ||
	    (c >= '0' && c <= '9') ||
	    c == '-' || c == '_' || c == '.' || c == ':');
}

/*
 * Returns true if this is a valid user-defined property (one with a ':').
 */
boolean_t
zfs_prop_user(const char *name)
{
	int i;
	char c;
	boolean_t foundsep = B_FALSE;

	for (i = 0; i < strlen(name); i++) {
		c = name[i];
		if (!valid_char(c))
			return (B_FALSE);
		if (c == ':')
			foundsep = B_TRUE;
	}

	if (!foundsep)
		return (B_FALSE);

	return (B_TRUE);
}

/*
 * Returns true if this is a valid userspace-type property (one with a '@').
 * Note that after the @, any character is valid (eg, another @, for SID
 * user@domain).
 */
boolean_t
zfs_prop_userquota(const char *name)
{
	zfs_userquota_prop_t prop;

	for (prop = 0; prop < ZFS_NUM_USERQUOTA_PROPS; prop++) {
		if (strncmp(name, zfs_userquota_prop_prefixes[prop],
		    strlen(zfs_userquota_prop_prefixes[prop])) == 0) {
			return (B_TRUE);
		}
	}

	return (B_FALSE);
}

/*
 * Returns true if this is a valid written@ property.
 * Note that after the @, any character is valid (eg, another @, for
 * written@pool/fs@origin).
 */
boolean_t
zfs_prop_written(const char *name)
{
	static const char *prefix = "written@";
	return (strncmp(name, prefix, strlen(prefix)) == 0);
}

/*
 * Tables of index types, plus functions to convert between the user view
 * (strings) and internal representation (uint64_t).
 */
int
zfs_prop_string_to_index(zfs_prop_t prop, const char *string, uint64_t *index)
{
	return (zprop_string_to_index(prop, string, index, ZFS_TYPE_DATASET));
}

int
zfs_prop_index_to_string(zfs_prop_t prop, uint64_t index, const char **string)
{
	return (zprop_index_to_string(prop, index, string, ZFS_TYPE_DATASET));
}

uint64_t
zfs_prop_random_value(zfs_prop_t prop, uint64_t seed)
{
	return (zprop_random_value(prop, seed, ZFS_TYPE_DATASET));
}

/*
 * Returns TRUE if the property applies to any of the given dataset types.
 */
boolean_t
zfs_prop_valid_for_type(int prop, zfs_type_t types, boolean_t headcheck)
{
	return (zprop_valid_for_type(prop, types, headcheck));
}

zprop_type_t
zfs_prop_get_type(zfs_prop_t prop)
{
	return (zfs_prop_table[prop].pd_proptype);
}

/*
 * Returns TRUE if the property is readonly.
 */
boolean_t
zfs_prop_readonly(zfs_prop_t prop)
{
	return (zfs_prop_table[prop].pd_attr == PROP_READONLY ||
	    zfs_prop_table[prop].pd_attr == PROP_ONETIME ||
	    zfs_prop_table[prop].pd_attr == PROP_ONETIME_DEFAULT);
}

/*
 * Returns TRUE if the property is visible (not hidden).
 */
boolean_t
zfs_prop_visible(zfs_prop_t prop)
{
	return (zfs_prop_table[prop].pd_visible);
}

/*
 * Returns TRUE if the property is only allowed to be set once.
 */
boolean_t
zfs_prop_setonce(zfs_prop_t prop)
{
	return (zfs_prop_table[prop].pd_attr == PROP_ONETIME ||
	    zfs_prop_table[prop].pd_attr == PROP_ONETIME_DEFAULT);
}

const char *
zfs_prop_default_string(zfs_prop_t prop)
{
	return (zfs_prop_table[prop].pd_strdefault);
}

uint64_t
zfs_prop_default_numeric(zfs_prop_t prop)
{
	return (zfs_prop_table[prop].pd_numdefault);
}

/*
 * Given a dataset property ID, returns the corresponding name.
 * Assuming the zfs dataset property ID is valid.
 */
const char *
zfs_prop_to_name(zfs_prop_t prop)
{
	return (zfs_prop_table[prop].pd_name);
}

/*
 * Returns TRUE if the property is inheritable.
 */
boolean_t
zfs_prop_inheritable(zfs_prop_t prop)
{
	return (zfs_prop_table[prop].pd_attr == PROP_INHERIT ||
	    zfs_prop_table[prop].pd_attr == PROP_ONETIME);
}

/*
 * Returns TRUE if property is one of the encryption properties that requires
 * a loaded encryption key to modify.
 */
boolean_t
zfs_prop_encryption_key_param(zfs_prop_t prop)
{
	/*
	 * keylocation does not count as an encryption property. It can be
	 * changed at will without needing the master keys.
	 */
	return (prop == ZFS_PROP_PBKDF2_SALT || prop == ZFS_PROP_PBKDF2_ITERS ||
	    prop == ZFS_PROP_KEYFORMAT);
}

/*
 * Helper function used by both kernelspace and userspace to check the
 * keylocation property. If encrypted is set, the keylocation must be valid
 * for an encrypted dataset.
 */
boolean_t
zfs_prop_valid_keylocation(const char *str, boolean_t encrypted)
{
	if (strcmp("none", str) == 0)
		return (!encrypted);
	else if (strcmp("prompt", str) == 0)
		return (B_TRUE);
	else if (strlen(str) > 8 && strncmp("file:///", str, 8) == 0)
		return (B_TRUE);

	return (B_FALSE);
}


#ifndef _KERNEL

/*
 * Returns a string describing the set of acceptable values for the given
 * zfs property, or NULL if it cannot be set.
 */
const char *
zfs_prop_values(zfs_prop_t prop)
{
	return (zfs_prop_table[prop].pd_values);
}

/*
 * Returns TRUE if this property is a string type.  Note that index types
 * (compression, checksum) are treated as strings in userland, even though they
 * are stored numerically on disk.
 */
int
zfs_prop_is_string(zfs_prop_t prop)
{
	return (zfs_prop_table[prop].pd_proptype == PROP_TYPE_STRING ||
	    zfs_prop_table[prop].pd_proptype == PROP_TYPE_INDEX);
}

/*
 * Returns the column header for the given property.  Used only in
 * 'zfs list -o', but centralized here with the other property information.
 */
const char *
zfs_prop_column_name(zfs_prop_t prop)
{
	return (zfs_prop_table[prop].pd_colname);
}

/*
 * Returns whether the given property should be displayed right-justified for
 * 'zfs list'.
 */
boolean_t
zfs_prop_align_right(zfs_prop_t prop)
{
	return (zfs_prop_table[prop].pd_rightalign);
}

#endif

#if defined(_KERNEL)
int
zcommon_init(void)
{
	fletcher_4_init();
	return (0);
}

void
zcommon_fini(void)
{
	fletcher_4_fini();
}
#endif
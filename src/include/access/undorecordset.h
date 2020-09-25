/*-------------------------------------------------------------------------
 *
 * undorecordset.h
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/access/undorecordset.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef UNDORECORDSET_H
#define UNDORECORDSET_H

#include "access/transam.h"
#include "access/undodefs.h"
#include "access/xlogreader.h"
#ifdef FRONTEND
#include "common/logging.h"
#endif

/*
 * Possible undo record set types. These are stored as 1-byte values on disk;
 * changing the values is an on-disk format break.
 */
typedef enum UndoRecordSetType
{
	URST_INVALID = 0,			/* Placeholder when there's no record set. */
	URST_TRANSACTION = 'T',		/* Normal xact undo; apply on abort. */
	URST_MULTI = 'M',			/* Informational undo; lives until every xact
								 * is all-visible or aborted and undone. */
	URST_EPHEMERAL = 'E',		/* Ephemeral data for testing purposes. */
	URST_FOO = 'F'				/* XXX. Crude hack; replace me. */
} UndoRecordSetType;

/*
 * The header that appears at the start of each 'chunk'.
 */
typedef struct UndoRecordSetChunkHeader
{
	UndoLogOffset	size;
	UndoRecPtr		previous_chunk;
	uint8			type;
} UndoRecordSetChunkHeader;

#define SizeOfUndoRecordSetChunkHeader \
	(offsetof(UndoRecordSetChunkHeader, type) + sizeof(uint8))

/* On-disk header for an UndoRecordSet of type URST_TRANSACTION. */
typedef struct XactUndoRecordSetHeader
{
	FullTransactionId	fxid;
	Oid					dboid;
} XactUndoRecordSetHeader;

/*
 * TODO Handle the missing types.
 */
static inline size_t
get_urs_type_header_size(UndoRecordSetType type)
{
	switch (type)
	{
		case URST_TRANSACTION:
			return sizeof(XactUndoRecordSetHeader);
		case URST_FOO:
			return 4;
		default:
#ifndef FRONTEND
			elog(FATAL, "unrecognized undo record set type %d", type);
#else
			pg_log_error("unrecognized undo record set type %d", type);
			exit(EXIT_FAILURE);
#endif
	}
}

extern UndoRecordSet *UndoCreate(UndoRecordSetType type, char presistence,
								 int nestingLevel, Size type_header_size,
								 char *type_header);
extern bool UndoPrepareToMarkClosed(UndoRecordSet *urs);
extern void UndoMarkClosed(UndoRecordSet *urs);
extern UndoRecPtr UndoPrepareToInsert(UndoRecordSet *urs, size_t record_size);
extern void UndoInsert(UndoRecordSet *urs,
					   void *record_data,
					   size_t record_size);
extern void UndoPageSetLSN(UndoRecordSet *urs, XLogRecPtr lsn);
extern void UndoRelease(UndoRecordSet *urs);
extern void UndoDestroy(UndoRecordSet *urs);
extern void UndoXLogRegisterBuffers(UndoRecordSet *urs, uint8 first_block_id);

/* recovery */
extern UndoRecPtr UndoReplay(XLogReaderState *xlog_record,
							 void *record_data,
							 size_t record_size);
extern void CloseDanglingUndoRecordSets(void);

/* transaction integration */
extern void UndoResetInsertion(void);
extern bool UndoPrepareToMarkClosedForXactLevel(int nestingLevel);
extern void UndoMarkClosedForXactLevel(int nestingLevel);
extern void UndoXLogRegisterBuffersForXactLevel(int nestingLevel,
												uint8 first_block_id);
extern void UndoPageSetLSNForXactLevel(int nestingLevel, XLogRecPtr lsn);
extern void UndoDestroyForXactLevel(int nestingLevel);
extern bool UndoCloseAndDestroyForXactLevel(int nestingLevel);

extern void AtProcExit_UndoRecordSet(void);

#endif
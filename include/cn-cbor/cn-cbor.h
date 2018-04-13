
/**
 * \file
 * \brief
 * CBOR parsing
 */

#ifndef CN_CBOR_H
#define CN_CBOR_H

#define CN_CBOR_VERSION 0x010100

#ifdef __MBED__
#include <stddef.h>
#endif

#ifndef CN_CBOR_EXPORT
#if defined(_WIN32)
#if defined(CN_CBOR_IS_DLL)
#define CN_CBOR_EXPORT __declspec(dllimport)
#else
#define CN_CBOR_EXPORT
#endif /* CN_CBOR_IS_DLL */
#else  /* defined (_WIN32) */
#define CN_CBOR_EXPORT
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
#ifdef EMACS_INDENTATION_HELPER
} /* Duh. */
#endif

#include <stdbool.h>
#include <stdint.h>
#ifdef _MSC_VER
#include <WinSock2.h>
typedef signed long ssize_t;
#else
#ifndef __MBED__
#include <unistd.h>
#else
#ifndef RETARGET_H
#ifndef _SSIZE_T_DECLARED
typedef signed long ssize_t;
#define _SSIZE_T_DECLARED
#endif
#endif
#endif
#endif

/**
 * All of the different kinds of CBOR values.
 */
typedef enum cn_cbor_type {
	/** false */
	CN_CBOR_FALSE,
	/** true */
	CN_CBOR_TRUE,
	/** null */
	CN_CBOR_NULL,
	/** undefined */
	CN_CBOR_UNDEF,
	/** Positive integers */
	CN_CBOR_UINT,
	/** Negative integers */
	CN_CBOR_INT,
	/** Byte string */
	CN_CBOR_BYTES,
	/** UTF-8 string */
	CN_CBOR_TEXT,
	/** Byte string, in chunks.  Each chunk is a child. */
	CN_CBOR_BYTES_CHUNKED,
	/** UTF-8 string, in chunks.  Each chunk is a child */
	CN_CBOR_TEXT_CHUNKED,
	/** Array of CBOR values.  Each array element is a child, in order */
	CN_CBOR_ARRAY,
	/** Map of key/value pairs.  Each key and value is a child, alternating. */
	CN_CBOR_MAP,
	/** Tag describing the next value.  The next value is the single child. */
	CN_CBOR_TAG,
	/** Simple value, other than the defined ones */
	CN_CBOR_SIMPLE,
#ifndef CBOR_NO_FLOAT
	/** Doubles, floats, and half-floats */
	CN_CBOR_DOUBLE,
	/** Floats, and half-floats */
	CN_CBOR_FLOAT,
#endif
	/** An error has occurred */
	CN_CBOR_INVALID
} cn_cbor_type;

/**
 * Flags used during parsing.  Not useful for consumers of the
 * `cn_cbor` structure.
 */
typedef enum cn_cbor_flags {
	/** The count field will be used for parsing */
	CN_CBOR_FL_COUNT = 1,
	/** An indefinite number of children */
	CN_CBOR_FL_INDEF = 2,
	/** Don't compress floating points value to smaller */
	CN_CBOR_FL_KEEP_FLOAT_SIZE = 4,
	/** This node was not allocated - don't free */
	CN_CBOR_FL_EXT_SELF = 0x40,
	/** The data is not owned by this node - don't free */
	CN_CBOR_FL_EXT_DATA = 0x80
} cn_cbor_flags;

/**
 * A CBOR value
 */
typedef struct cn_cbor {
	/** The type of value */
	cn_cbor_type type;
	/** Flags used at parse time */
	cn_cbor_flags flags;
	/** Data associated with the value; different branches of the union are
		used depending on the `type` field. */
	union {
		/** CN_CBOR_BYTES */
		const uint8_t* bytes;
		/** CN_CBOR_TEXT */
		const char* str;
		/** CN_CBOR_INT */
#ifdef _MSC_VER
		int64_t sint;
#else
		long sint;
#endif
		/** CN_CBOR_UINT */
#ifdef _MSC_VER
		uint64_t uint;
#else
		unsigned long uint;
#endif
#ifndef CBOR_NO_FLOAT
		/** CN_CBOR_DOUBLE */
		double dbl;
		/** CN_CBOR_FLOAT */
		float f;
#endif
		/** for use during parsing */
		size_t count;
	} v; /* TBD: optimize immediate */
	/** Number of children.
	 * @note: for maps, this is 2x the number of entries */
	size_t length;
	/** The first child value */
	struct cn_cbor* first_child;
	/** The last child value */
	struct cn_cbor* last_child;
	/** The sibling after this one, or NULL if this is the last */
	struct cn_cbor* next;
	/** The parent of this value, or NULL if this is the root */
	struct cn_cbor* parent;
} cn_cbor;

/**
 * All of the different kinds of errors
 */
typedef enum cn_cbor_error {
	/** No error has occurred */
	CN_CBOR_NO_ERROR,
	/** More data was expected while parsing */
	CN_CBOR_ERR_OUT_OF_DATA,
	/** Some extra data was left over at the end of parsing */
	CN_CBOR_ERR_NOT_ALL_DATA_CONSUMED,
	/** A map should be alternating keys and values.  A break was found
		when a value was expected */
	CN_CBOR_ERR_ODD_SIZE_INDEF_MAP,
	/** A break was found where it wasn't expected */
	CN_CBOR_ERR_BREAK_OUTSIDE_INDEF,
	/** Indefinite encoding works for bstrs, strings, arrays, and maps.
		A different major type tried to use it. */
	CN_CBOR_ERR_MT_UNDEF_FOR_INDEF,
	/** Additional Information values 28-30 are reserved */
	CN_CBOR_ERR_RESERVED_AI,
	/** A chunked encoding was used for a string or bstr, and one of the elements
		wasn't the expected (string/bstr) type */
	CN_CBOR_ERR_WRONG_NESTING_IN_INDEF_STRING,
	/** An invalid parameter was passed to a function */
	CN_CBOR_ERR_INVALID_PARAMETER,
	/** Allocation failed */
	CN_CBOR_ERR_OUT_OF_MEMORY,
	/** A float was encountered during parse but the library was built without
		support for float types. */
	CN_CBOR_ERR_FLOAT_NOT_SUPPORTED
} cn_cbor_error;

/**
 * Strings matching the `cn_cbor_error` conditions.
 *
 * @todo: turn into a function to make the type safety more clear?
 */
CN_CBOR_EXPORT
extern const char* cn_cbor_error_str[];

/**
 * Errors
 */
typedef struct cn_cbor_errback {
	/** The position in the input where the error happened */
	size_t pos;
	/** The error, or CN_CBOR_NO_ERROR if none */
	cn_cbor_error err;
} cn_cbor_errback;

#ifdef USE_CBOR_CONTEXT

/**
 * Allocate and zero out memory.  `count` elements of `size` are required,
 * as for `calloc(3)`.  The `context` is the `cn_cbor_context` passed in
 * earlier to the CBOR routine.
 *
 * @param[in] count   The number of items to allocate
 * @param[in] size    The size of each item
 * @param[in] context The allocation context
 */
typedef void* (*cn_calloc_func)(size_t count, size_t size, void* context);

/**
 * Free memory previously allocated with a context.  If using a pool allocator,
 * this function will often be a no-op, but it must be supplied in order to
 * prevent the CBOR library from calling `free(3)`.
 *
 * @note: it may be that this is never needed; if so, it will be removed for
 * clarity and speed.
 *
 * @param  context [description]
 * @return         [description]
 */
typedef void (*cn_free_func)(void* ptr, void* context);

/**
 * The allocation context.
 */
typedef struct cn_cbor_context {
	/** The pool `calloc` routine.  Must allocate and zero. */
	cn_calloc_func calloc_func;
	/** The pool `free` routine.  Often a no-op, but required. */
	cn_free_func free_func;
	/** Typically, the pool object, to be used when calling `calloc_func`
	 * and `free_func` */
	void* context;
} cn_cbor_context;

/** When USE_CBOR_CONTEXT is defined, many functions take an extra `context`
 * parameter */
#define CBOR_CONTEXT , const cn_cbor_context* context
/** When USE_CBOR_CONTEXT is defined, some functions take an extra `context`
 * parameter at the beginning */
#define CBOR_CONTEXT_COMMA const cn_cbor_context *context,

#else

#define CBOR_CONTEXT
#define CBOR_CONTEXT_COMMA

#endif

/**
 * Tag the data as not to be freed
 *
 */

CN_CBOR_EXPORT
void cn_cbor_dont_free_data(cn_cbor* cbor);

/**
 * Decode an array of CBOR bytes into structures.
 *
 * @note Pointers to the buffer are placed into the structure provided.
 * This means that the buffer needs to be kept as long as the decoded
 * structure is kept.
 *
 * @param[in]  buf          The array of bytes to parse
 * @param[in]  len          The number of bytes in the array
 * @param[in]  CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out] errp         Error, if NULL is returned
 * @return                  The parsed CBOR structure, or NULL on error
 */
CN_CBOR_EXPORT
cn_cbor* cn_cbor_decode(const uint8_t* buf, size_t len CBOR_CONTEXT, cn_cbor_errback* errp);

/**
 * Get a value from a CBOR map that has the given string as a key.
 *
 * @param[in]  cb           The CBOR map
 * @param[in]  key          The string to look up in the map
 * @return                  The matching value, or NULL if the key is not found
 */
CN_CBOR_EXPORT
cn_cbor* cn_cbor_mapget_string(const cn_cbor* cb, const char* key);

/**
 * Get a value from a CBOR map that has the given integer as a key.
 *
 * @param[in]  cb           The CBOR map
 * @param[in]  key          The int to look up in the map
 * @return                  The matching value, or NULL if the key is not found
 */
CN_CBOR_EXPORT
cn_cbor* cn_cbor_mapget_int(const cn_cbor* cb, int key);

/**
 * Get the item with the given index from a CBOR array.
 *
 * @param[in]  cb           The CBOR array
 * @param[in]  idx          The array index
 * @return                  The matching value, or NULL if the index is invalid
 */
CN_CBOR_EXPORT
cn_cbor* cn_cbor_index(const cn_cbor* cb, unsigned int idx);

/**
 * Free the given CBOR structure.
 * You MUST NOT try to free a cn_cbor structure with a parent (i.e., one
 * that is not a root in the tree).
 *
 * @param[in]  cb           The CBOR value to free.  May be NULL, or a root object.
 * @param[in]  CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 */
CN_CBOR_EXPORT
void cn_cbor_free(cn_cbor* cb CBOR_CONTEXT);

/**
 * Write a CBOR value and all of the child values.
 *
 * @param[in]  buf        The buffer into which to write. May be NULL to
 *                        determine the necessary size.
 * @param[in]  buf_offset The offset (in bytes) from the beginning of the buffer
 *                        to start writing at
 * @param[in]  buf_size   The total length (in bytes) of the buffer. If buf is
 *                        NULL, this is an upper limit and may be 0 to specify
 *                        no limit.
 * @param[in]  cb         [description]
 * @return                -1 on fail, or number of bytes written
 */
ssize_t cn_cbor_encoder_write(uint8_t* buf, size_t buf_offset, size_t buf_size, const cn_cbor* cb);

/**
 * Create a CBOR map.
 *
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created map, or NULL on error
 */
CN_CBOR_EXPORT
cn_cbor* cn_cbor_map_create(CBOR_CONTEXT_COMMA cn_cbor_errback* errp);

/**
 * Create a CBOR byte string.  The data in the byte string is *not* owned
 * by the CBOR object, so it is not freed automatically.
 *
 * @param[in]   data         The data
 * @param[in]   len          The number of bytes of data
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
CN_CBOR_EXPORT
cn_cbor* cn_cbor_data_create(const uint8_t* data, int len CBOR_CONTEXT, cn_cbor_errback* errp);

/**
 * Create a CBOR byte string.  Ownership of the passed in data is
 * controlled by the flags.  Default is ownership is given up to CBOR.
 *
 * @param[in]   data         The data
 * @param[in]   len          The number of bytes of data
 * @param[in]   flags        CN_CBOR_FL_EXT_DATA or 0
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
CN_CBOR_EXPORT
cn_cbor* cn_cbor_data_create2(const uint8_t* data, int len, int flags CBOR_CONTEXT, cn_cbor_errback* errp);

/**
 * Create a CBOR UTF-8 string.  The data is not checked for UTF-8 correctness.
 * The data being stored in the string is *not* owned the CBOR object, so it is
 * not freed automatically.
 *
 * @note: Do NOT use this function with untrusted data.  It calls strlen, and
 * relies on proper NULL-termination.
 *
 * @param[in]   data         NULL-terminated UTF-8 string
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
CN_CBOR_EXPORT
cn_cbor* cn_cbor_string_create(const char* data, CBOR_CONTEXT_COMMA cn_cbor_errback* errp);

/**
 * Create a CBOR UTF-8 string.  The data is not checked for UTF-8 correctness.
 * Ownership of the passed in data is controlled by the flags.
 * Default is ownership is given up to CBOR.
 *
 * @note: Do NOT use this function with untrusted data.  It calls strlen, and
 * relies on proper NULL-termination.
 *
 * @param[in]   data         NULL-terminated UTF-8 string
 * @param[in]   flags        CN_CBOR_FL_EXT_DATA or 0
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
CN_CBOR_EXPORT
cn_cbor* cn_cbor_string_create2(const char* data, int flags CBOR_CONTEXT, cn_cbor_errback* errp);

/**
 * Create a CBOR integer (either positive or negative).
 *
 * @param[in]   value    the value of the integer
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
CN_CBOR_EXPORT
cn_cbor* cn_cbor_int_create(int64_t value CBOR_CONTEXT, cn_cbor_errback* errp);

#ifndef CBOR_NO_FLOAT
/**
 * Create a CBOR float.
 *
 * @param[in]   value    the value of the float
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
cn_cbor* cn_cbor_float_create(float value CBOR_CONTEXT, cn_cbor_errback* errp);

/**
 * Create a CBOR double.
 *
 * @param[in]   value    the value of the double
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
cn_cbor* cn_cbor_double_create(double value CBOR_CONTEXT, cn_cbor_errback* errp);
#endif /* CBOR_NO_FLOAT */

/**
 * Create a CBOR simple value.
 *
 * @note: Do NOT use this function with untrusted data.  It calls strlen, and
 * relies on proper NULL-termination.
 *
 * @param[in]   simpleValue  Simple value to have
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
cn_cbor* cn_cbor_simple_create(int simpleValue, CBOR_CONTEXT_COMMA cn_cbor_errback* errp);

/**
 * Create a CBOR NULL
 *
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
static inline cn_cbor* cn_cbor_null_create(CBOR_CONTEXT_COMMA cn_cbor_errback* errp)
{
	return cn_cbor_simple_create(22,
#ifdef USE_CBOR_CONTEXT
		context,
#endif
		errp);
}

/**
 * Tag a CBOR object
 *
 * @param[in]   tag          Tag to be added
 * @param[in]   child        child data to this object
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[in]   perr         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */

cn_cbor* cn_cbor_tag_create(int tag, cn_cbor* child, CBOR_CONTEXT_COMMA cn_cbor_errback* perr);

/**
 * Create a CBOR UTF-8 string.  The data is not checked for UTF-8 correctness.
 * The data being stored in the string is *not* owned the CBOR object, so it is
 * not freed automatically.
 *
 * @note: Do NOT use this function with untrusted data.  It calls strlen, and
 * relies on proper NULL-termination.
 *
 * @param[in]   value        true or false
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */

static inline cn_cbor* cn_cbor_bool_create(bool value CBOR_CONTEXT, cn_cbor_errback* errp)
{
	return cn_cbor_simple_create(value ? 21 : 20,
#ifdef USE_CBOR_CONTEXT
		context,
#endif
		errp);
}

/**
 * Create a chunked text or byte string.
 *
 * @param[in]   type          CN_CBOR_BYTES or CN_CBOR_TEXT
 * @param[in]   CBOR_CONTEXT  Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[in]   errp          Error; if NULL is returned
 * @return                    The created object or NULL on error
 */
CN_CBOR_EXPORT
cn_cbor* cn_cbor_chunked_create(int type, CBOR_CONTEXT_COMMA cn_cbor_errback* errp);

/**
 * Append a node to a chunked list.
 *
 * @param[in]   cb_array      Node tagged as chunked
 * @param[in]   cb_value      Item to be appended
 * @param[in]   errp          Error; if FALSE is returned
 * @return                    True if successfully appended
 */
CN_CBOR_EXPORT
bool cn_cbor_chunked_append(cn_cbor* cb_array, cn_cbor* cb_value, cn_cbor_errback* errp);

/**
 * Put a CBOR object into a map with a CBOR object key.  Duplicate checks are NOT
 * currently performed.
 *
 * @param[in]   cb_map       The map to insert into
 * @param[in]   cb_key          The key
 * @param[in]   cb_value     The value
 * @param[out]  errp         Error
 * @return                   True on success
 */
CN_CBOR_EXPORT
bool cn_cbor_map_put(cn_cbor* cb_map, cn_cbor* cb_key, cn_cbor* cb_value, cn_cbor_errback* errp);

/**
 * Put a CBOR object into a map with an integer key.  Duplicate checks are NOT
 * currently performed.
 *
 * @param[in]   cb_map       The map to insert into
 * @param[in]   key          The integer key
 * @param[in]   cb_value     The value
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error
 * @return                   True on success
 */
CN_CBOR_EXPORT
bool cn_cbor_mapput_int(cn_cbor* cb_map, int64_t key, cn_cbor* cb_value CBOR_CONTEXT, cn_cbor_errback* errp);

/**
 * Put a CBOR object into a map with a string key.  Duplicate checks are NOT
 * currently performed.  The string value is not freed when the object is freed.
 *
 * @note: do not call this routine with untrusted string data.  It calls
 * strlen, and requires a properly NULL-terminated key.
 *
 * @param[in]   cb_map       The map to insert into
 * @param[in]   key          The string key
 * @param[in]   cb_value     The value
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error
 * @return                   True on success
 */
CN_CBOR_EXPORT
bool cn_cbor_mapput_string(cn_cbor* cb_map, const char* key, cn_cbor* cb_value CBOR_CONTEXT, cn_cbor_errback* errp);

/**
 * Put a CBOR object into a map with a string key.  Duplicate checks are NOT
 * currently performed.  The string will be freed depending on the flags.
 *
 * @note: do not call this routine with untrusted string data.  It calls
 * strlen, and requires a properly NULL-terminated key.
 *
 * @param[in]   cb_map       The map to insert into
 * @param[in]   key          The string key
 * @param[in]   cb_value     The value
 * @param[in]   flags        CN_CBOR_FL_EXT_DATA | 0
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error
 * @return                   True on success
 */
CN_CBOR_EXPORT
bool cn_cbor_mapput_string2(cn_cbor* cb_map,
	const char* key,
	cn_cbor* cb_value,
	int flags CBOR_CONTEXT,
	cn_cbor_errback* errp);

/**
 * Create a CBOR array
 *
 * @param[in]   CBOR_CONTEXT Allocation context (only if USE_CBOR_CONTEXT is defined)
 * @param[out]  errp         Error, if NULL is returned
 * @return                   The created object, or NULL on error
 */
CN_CBOR_EXPORT
cn_cbor* cn_cbor_array_create(CBOR_CONTEXT_COMMA cn_cbor_errback* errp);

/**
 * Append an item to the end of a CBOR array.
 *
 * @param[in]   cb_array  The array into which to insert
 * @param[in]   cb_value  The value to insert
 * @param[out]  errp      Error
 * @return                True on success
 */
CN_CBOR_EXPORT
bool cn_cbor_array_append(cn_cbor* cb_array, cn_cbor* cb_value, cn_cbor_errback* errp);

/**
 * Dump the object to a file pointer
 * If buffer is NULL, then return required size to generate output
 *
 * @param[in]   buffer	Location to place output
 * @param[in]   bufferSize Size of return buffer
 * @param[in]	cb		tree to be dumped
 * @param[in]   indent  string to use for each level of indention
 * @param[in]   crlf    string to use for end of line marker
 * @return				size of output generated, -1 if buffer is too small
 */

extern ssize_t cn_cbor_printer_write(char* buffer,
	size_t bufferSize,
	const cn_cbor* cb,
	const char* indent,
	const char* crlf);

#ifdef __MBED__
#define ntohs(a) ((uint16_t)(((((uint16_t)(a)) & 0xff) << 8) | (((uint16_t)(a)) & 0xff00) >> 8))
#define htons(a) ntohs(a)
#define ntohl(a)                                                                                 \
	((uint32_t)(((((uint32_t)(a)) & 0x000000ff) << 24) | ((((uint32_t)(a)) & 0x0000ff00) << 8) | \
				((((uint32_t)(a)) & 0x00ff0000) >> 8) | ((((uint32_t)(a)) & 0xff000000) >> 24)))
#define htonl(a) ntohl(a)
#endif	// __MBED__

#ifdef __cplusplus
}
#endif

#endif /* CN_CBOR_H */

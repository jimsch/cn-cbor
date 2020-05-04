#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#else
#endif
#include <string.h>

#include "cn-cbor/cn-cbor.h"

#ifdef USE_CBOR_CONTEXT
#include "context.h"
#define CBOR_CONTEXT_PARAM , NULL
#else
#define CBOR_CONTEXT_PARAM
#endif

int CFails;

#ifdef USE_CBOR_CONTEXT
void CreateTests()
{
	cn_cbor_context* context = NULL;

	
	context = CreateContext(-1);

	//  Check the simple create/delete for memory leaks.

	cn_cbor* cbor = cn_cbor_map_create(context, NULL);
	cn_cbor_free(cbor, context);

	uint8_t* pb = (uint8_t*)context->calloc_func(10, 10, context);
	cbor = cn_cbor_data_create2(pb, 10, 0, context, NULL);
	cn_cbor_free(cbor, context);

	char* sz = (char*)context->calloc_func(10, 1, context);
	strcpy(sz, "ABC");
	cbor = cn_cbor_string_create2(sz, 0, context, NULL);
	cn_cbor_free(cbor, context);

	cbor = cn_cbor_string_create("This is a string", context, NULL);
	cn_cbor_dont_free_data(cbor);
	cn_cbor_free(cbor, context);

	cbor = cn_cbor_int_create(20, context, NULL);
	cn_cbor_free(cbor, context);

#ifndef CBOR_NO_FLOAT
	cbor = cn_cbor_float_create((float)20.2, context, NULL);
	cn_cbor_free(cbor, context);

	cbor = cn_cbor_double_create(203.3, context, NULL);
	cn_cbor_free(cbor, context);
#endif

	cbor = cn_cbor_array_create(context, NULL);
	cn_cbor_free(cbor, context);

	cbor = cn_cbor_bool_create(false, context, NULL);
	cn_cbor_free(cbor, context);

	cbor = cn_cbor_null_create(context, NULL);
	cn_cbor_free(cbor, context);

	cbor = cn_cbor_simple_create(23, context, NULL);
	cn_cbor_free(cbor, context);

	if (IsContextEmpty(context) > 0) {
		CFails += 1;
	}

	//  Test more complex stuctures

	cn_cbor* cbor_map = cn_cbor_map_create(context, NULL);
	cn_cbor* cbor_array = cn_cbor_array_create(context, NULL);
	cn_cbor* cbor2 = NULL;

	for (int i = 0; i < 10; i++) {
		cbor2 = cn_cbor_int_create(i, context, NULL);
		cbor = cn_cbor_int_create(i, context, NULL);
		cn_cbor_map_put(cbor_map, cbor, cbor2, NULL);

		cbor = cn_cbor_int_create(i, context, NULL);
		cn_cbor_array_append(cbor_array, cbor, NULL);
	}

	cn_cbor_mapput_string(cbor_map, "KEY", cn_cbor_int_create(20, context, NULL), context, NULL);
	sz = (char*)context->calloc_func(10, 1, context);
	strcpy(sz, "ABC");
	cn_cbor_mapput_string2(cbor_map, sz, cn_cbor_int_create(-20, context, NULL), 0, context, NULL);

	cn_cbor_mapput_int(cbor_map, -22, cn_cbor_simple_create(99, context, NULL), context, NULL);

	cn_cbor_array_append(cbor_array, cbor_map, NULL);
	cbor_array = cn_cbor_tag_create(99, cbor_array, context, NULL);
	cn_cbor_free(cbor_array, context);

	if (IsContextEmpty(context) > 0) {
		CFails += 1;
	}

	
}

void DecoderTests() {}

void EncoderTests()
{
	bool finished = false;
	for (int passNumber = 0; passNumber < 10000 && !finished; passNumber++) {
		cn_cbor* cborRoot = NULL;
		cn_cbor* cbor = NULL;
		cn_cbor* cbor2 = NULL;
		uint8_t* pb = NULL;
		char* s = NULL;

		cn_cbor_context* context = CreateContext(passNumber);
		if (context == NULL) {
			CFails += 1;
			return;
		}

		cborRoot = cn_cbor_array_create(context, NULL);
		if (cborRoot == NULL) {
			goto errorReturn;
		}

		cbor = cn_cbor_array_create(context, NULL);
		if (cbor == NULL) {
			goto errorReturn;
		}
		cbor->flags |= CN_CBOR_FL_INDEF;

		cbor2 = cn_cbor_simple_create(22, context, NULL);
		if (cbor2 == NULL) {
			goto errorReturn;
		}
		cn_cbor_array_append(cbor, cbor2, NULL);
		cbor2 = NULL;

		cbor2 = cn_cbor_simple_create(21, context, NULL);
		if (cbor2 == NULL) {
			goto errorReturn;
		}
		cn_cbor_array_append(cbor, cbor2, NULL);
		cbor2 = NULL;

		cn_cbor_array_append(cborRoot, cbor, NULL);
		cbor = NULL;

		cbor = cn_cbor_bool_create(true, context, NULL);
		if (cbor == NULL) {
			goto errorReturn;
		}
		cn_cbor_array_append(cborRoot, cbor, NULL);
		cbor = NULL;

		cbor = cn_cbor_map_create(context, NULL);
		if (cbor == NULL) {
			goto errorReturn;
		}

		cbor2 = cn_cbor_string_create("Text1", context, NULL);
		if (cbor2 == NULL) {
			goto errorReturn;
		}
		if (!cn_cbor_mapput_int(cbor, 5, cbor2, context, NULL)) {
			goto errorReturn;
		};
		cbor2 = NULL;

		cbor2 = cn_cbor_int_create(99, context, NULL);
		if (cbor2 == NULL) {
			goto errorReturn;
		}
		if (!cn_cbor_mapput_string(cbor, "key", cbor2, context, NULL)) {
			goto errorReturn;
		}
		cbor2 = NULL;

		cn_cbor_array_append(cborRoot, cbor, NULL);
		cbor = NULL;

		cbor = cn_cbor_map_create(context, NULL);
		if (cbor == NULL) {
			goto errorReturn;
		}
		cbor->flags |= CN_CBOR_FL_INDEF;

		cbor2 = cn_cbor_string_create("Text1", context, NULL);
		if (cbor2 == NULL) {
			goto errorReturn;
		}
		if (!cn_cbor_mapput_int(cbor, 5, cbor2, context, NULL)) {
			goto errorReturn;
		}
		cbor2 = NULL;

		cbor2 = cn_cbor_int_create(99, context, NULL);
		if (cbor2 == NULL) {
			goto errorReturn;
		}
		
		if (!cn_cbor_mapput_string(cbor, "key", cbor2, context, NULL)) {
			goto errorReturn;
		}
		cbor2 = NULL;

		cn_cbor_array_append(cborRoot, cbor, NULL);
		cbor = NULL;

		cbor = cn_cbor_chunked_create(CN_CBOR_BYTES, context, NULL);
		if (cbor == NULL) {
			goto errorReturn;
		}
		pb = context->calloc_func(10, 10, context);
		cbor2 = cn_cbor_data_create2(pb, 100, 0, context, NULL);
		if (cbor2 == NULL) {
			goto errorReturn;
		}
		pb = NULL;

		cn_cbor_chunked_append(cbor, cbor2, NULL);
		cbor2 = NULL;

		uint8_t data2[20] = {1, 2, 3, 4, 5, 6, 7};
		cbor2 = cn_cbor_data_create(data2, 20, context, NULL);
		if (cbor2 == NULL) {
			goto errorReturn;
		}
		cn_cbor_chunked_append(cbor, cbor2, NULL);
		cbor2 = NULL;

		cn_cbor_array_append(cborRoot, cbor, NULL);
		cbor = NULL;

		cbor = cn_cbor_chunked_create(CN_CBOR_TEXT, context, NULL);
		if (cbor == NULL) {
			goto errorReturn;
		}

		cbor2 = cn_cbor_string_create("This is a string", context, NULL);
		if (cbor2 == NULL) {
			goto errorReturn;
		}
		cn_cbor_chunked_append(cbor, cbor2, NULL);
		cbor2 = NULL;

		s = context->calloc_func(20, 1, context);
		if (s == NULL) {
			goto errorReturn;
		}
		strcpy(s, "Hi Mom");
		cbor2 = cn_cbor_string_create2(s, 0, context, NULL);
		if (cbor2 == NULL) {
			goto errorReturn;
		}
		s = NULL;

		cn_cbor_chunked_append(cbor, cbor2, NULL);
		cbor2 = NULL;

		cn_cbor_array_append(cborRoot, cbor, NULL);
		cbor = NULL;

		cbor = cn_cbor_simple_create(4, context, NULL);
		if (cbor == NULL) {
			goto errorReturn;
		}
		cbor2 = cn_cbor_tag_create(99, cbor, context, NULL);
		if (cbor2 == NULL) {
			goto errorReturn;
		}
		cbor = NULL;
		cn_cbor_array_append(cborRoot, cbor2, NULL);
		cbor2 = NULL;

#ifndef CBOR_NO_FLOAT
		cbor = cn_cbor_float_create(9, context, NULL);
		if (cbor == NULL) {
			goto errorReturn;
		}
		cn_cbor_array_append(cborRoot, cbor, NULL);
		cbor = NULL;

		cbor = cn_cbor_double_create(33.225932523223, context, NULL);
		if (cbor == NULL) {
			goto errorReturn;
		}
		cn_cbor_array_append(cborRoot, cbor, NULL);
		cbor = NULL;

		cbor = cn_cbor_float_create(9, context, NULL);
		if (cbor == NULL) {
			goto errorReturn;
		}
		cbor->flags |= CN_CBOR_FL_KEEP_FLOAT_SIZE;
		cn_cbor_array_append(cborRoot, cbor, NULL);
		cbor = NULL;

		cbor = cn_cbor_double_create(9, context, NULL);
		if (cbor == NULL) {
			goto errorReturn;
		}
		cbor->flags |= CN_CBOR_FL_KEEP_FLOAT_SIZE;
		cn_cbor_array_append(cborRoot, cbor, NULL);
		cbor = NULL;
#endif

		ssize_t cb = cn_cbor_encoder_write(NULL, 0, 0, cborRoot);
		pb = (uint8_t*)context->calloc_func(cb + 2, 1, context);
		if (pb == NULL) {
			goto errorReturn;
		}

		ssize_t cb2 = cn_cbor_encoder_write(pb, 0, cb - 1, cborRoot);
		if (cb2 != -1) {
			CFails += 1;
		}

		cb2 = cn_cbor_encoder_write(pb, 0, cb, cborRoot);
		if (cb2 != cb) {
			CFails += 1;
		}

		cb2 = cn_cbor_encoder_write(pb, 0, cb + 1, cborRoot);
		if (cb != cb2) {
			CFails += 1;
		}

		cn_cbor_free(cborRoot, context);
		cborRoot = NULL;

		cborRoot = cn_cbor_decode(pb, cb2, context, NULL);
		if (cborRoot == NULL) {
			goto errorReturn;
		}

		finished = true;

	errorReturn:
		if (cborRoot != NULL) {
			cn_cbor_free(cborRoot, context);
		}
		if (cbor != NULL) {
			cn_cbor_free(cbor, context);
		}
		if (cbor2 != NULL) {
			cn_cbor_free(cbor2, context);
		}
		if (pb != NULL) {
			context->free_func(pb, context);
		}
		if (s != NULL) {
			context->free_func(s, context);
		}

		if (IsContextEmpty(context) > 0) {
			CFails += 1;
		}

    FreeContext(context);
	}
}
#endif

int main(void)
{
#ifdef USE_CBOR_CONTEXT
	CreateTests();
	EncoderTests();
	DecoderTests();
#endif
	return CFails;
}

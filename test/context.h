#pragma once

#ifdef USE_CBOR_CONTEXT

extern cn_cbor_context* CreateContext(unsigned int failAt);
extern void FreeContext(cn_cbor_context* pContext);
int IsContextEmpty(const cn_cbor_context* pContext);

#endif	// USE_CBOR_CONTEXT

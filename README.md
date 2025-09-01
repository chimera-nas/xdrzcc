<!--
SPDX-FileCopyrightText: 2025 Ben Jarvis

SPDX-License-Identifier: LGPL-2.1-only
-->

# xdrzcc (eXternal Data Representation Zero Copy Compiler)

xdrzcc compiles external data representation (XDR) definitions into ANSI C code, similar to the venerable __rpcgen__.   The generated code provides functions to marshall and unmarshall XDR data types to and from plain old C data structures. xdrzcc aims to do this more efficiently than other available tools.

In particular, xdrzcc generates C code with the following properties:

* The serialized data stream is represented as an I/O vector array (iovec) style structure
* Opaque data blobs within the XDR data types can be marshalled and unmarshalled by reference without a memory copy. 
* Per-operation memory allocation can be completely avoided
* Additional metadata associated with memory buffers may be passed through the process to allow for RDMA or VFIO memory registration
* The generated C code has no external dependencies beyond libc and is structured to allow liberal inlining and loop unroll as appropriate

The primary motivation for creating xdrzcc is to use these capabilities to parse Network File System (NFS) traffic with high efficiency.   In an NFS data stream, the majority of the data is often opaque file content inside NFS read and write operations.   xdrzcc provides a way to parse these messages and then issue I/O requests to storage to/from the opaque file blobs without the tax of an additional memory copy.

## Status

xdrzcc is mostly complete but is still currently in an experimental state.    Expect to find some bugs if you intend to use it.  The API is not necessarily fully stable yet.

## Usage

To use xdrzcc, provide an XDR .x file and it will produce a C source file and header file:

```
xdrzcc <xdr.x> <output.c> <output.h>
```

## Example

Suppose we have an XDR definitions file example.x as follows:

```
struct MyMsg {
    unsigned int somevalue;
    string       astring;
    opaque       somedata<>;
};
```

We compile it as follows:

```
$ xdrzcc example.x example_xdr.c example_xdr.h
$ ls
example.x  example_xdr.c  example_xdr.h
```

The generated example_xdr.h will contain a C structure definition for MyMsg:

```c
typedef struct MyMsg MyMsg;

xdr_dbuf *xdr_dbuf_alloc(void); /* Allocate an unmarshalling scratch buffer */
void xdr_dbuf_free(xdr_dbuf *dbuf); /* Free an unmarshalling scratch buffer */

/* Reset scratch buffer */
static inline void
xdr_dbuf_reset(xdr_dbuf *dbuf)
{
    dbuf->used = 0;
}
struct MyMsg {
    uint32_t                                 somevalue;
    xdr_string                               astring;
    xdr_iovecr                               somedata;
};

/* Marshalls an array of MyMsg into a serialized i/o vector array
 * Returns the size of the marshalled encoding, or -1 on error
 */

int marshall_MyMsg(
    const MyMsg    *in,         /* input array of MyMsg to marshall */
    int 				 		 n,         /* number of input MyMsg to marshall */
    const xdr_iovec *iov_in,    /* Buffers in which to marshall output */
    int              niov_in,   /* Number of buffers available to marshall into */
    xdr_iovec       *iov_out,   /* Output iovecs */
    int             *niov_out); /* Number of output iovecs */

int unmarshall_MyMsg(
    MyMsg           *out,       /* output array of MyMsgh to be unmarshalled */
    int              n,         /* number of output MyMsg to unmarshall */
    const xdr_iovec *iov,       /* I/O vector array of marshalled input data */
    int              niov,      /* number of I/O vectors of input data */
    xdr_dbuf        *dbuf);     /* Scratch buffer for non-opaque content */
```

xdrzcc generated marshalling code strictly reads from the msg structures and writes to the output buffers.   In the case of opaque payloads, the output IOV will contain references to the input messages.   Therefore the msgs must remain in memory for the lifetime of any serialization produced from them. 

Similarly, xdrzc generated unmarshalling code will generate msg structures that contain references to the original serialization buffer.  Therefore the serialization buffer must remain in memory for the lifetime of any messages unmarshalled from it.  When unmarshalling, an xdr_dbuf scratch buffer must also be provided.  This buffer is internally resized as needed and contains the byte-order swapped contents of the non-opaque members of the messages.   The dbuf that is used to unmarshall a message must also remain intact for the lifetime of the resulting message.   To avoid runtime memory buffer allocation, the xdr_dbuf may be reset and reused once any previously unmarshalled messages have been destroyed.

## Known Issues and Limitations

* The parsing code does not have great error handling for things like syntax errors in the .x source.   XDR is frankly kind of a dead language.  xdrzcc's purpose is therefore to parse well known XDR specifications out of things like NFS RFCs that do not contain XDR syntax errors, not so much to support development of new XDR  use cases.
* xdrzcc assumes the native floating point and double-precision format of the system matches that required over the wire in XDR.  AFAIK this is the case for all common CPUs.

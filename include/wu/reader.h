#ifndef WU_READER_H
#define WU_READER_H

#include <stdio.h>
#include "types.h"
#include "data.h"

/**
 * Base for a reader, which defines the minimal interface for reading the
 * next data point from a data source and a method to free the reader's
 * resources. The delete method should be called by the runner taking
 * ownership of the reader. It is expected that specific reader
 * implementations will extend this base structure and implement the
 * defined methods.
 */
typedef struct Reader_ {
    void* (*next)(struct Reader_* reader);
    void (*delete)(struct Reader_* reader);
}* Reader;

#define reader_next(reader) (((Reader)(reader))->next((Reader)(reader)))

#define reader_last_error(reader) ((reader)->last_error)

#define reader_delete(reader) do { \
    if ((reader)->delete) \
        (reader)->delete((Reader)(reader)); \
} while(0)

#define CSV_MAX_LINE_SIZE 1024

typedef enum {
    CSV_OK = 0,
    CSV_ERROR_EOF = 1,
    CSV_ERROR_PARSE = 2
} CsvError;

/**
 * CsvReader is a concrete implementation of the Reader interface that
 * reads data from a CSV file.
 */
typedef struct CsvReader_ {
    struct Reader_ base;
    FILE* file;
    char line_buffer[CSV_MAX_LINE_SIZE];
    bool has_headers;
    DataType data_type;
    CsvError last_error;
    union {
        Candle candle;
        Trade trade;
        SingleValue single_value;
    } data;
}* CsvReader;

CsvReader csv_reader_new(FILE *file, DataType data_type, bool has_headers);

#endif // WU_READER_H

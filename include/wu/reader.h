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
typedef struct WU_Reader_ {
    void* (*next)(struct WU_Reader_* reader);
    void (*delete)(struct WU_Reader_* reader);
}* WU_Reader;

#define wu_reader_next(reader) (((WU_Reader)(reader))->next((WU_Reader)(reader)))

#define wu_reader_last_error(reader) ((reader)->last_error)

#define wu_reader_delete(reader) do { \
    if ((reader)->delete) \
        (reader)->delete((WU_Reader)(reader)); \
} while(0)

#define WU_CSV_MAX_LINE_SIZE 1024

typedef enum {
    WU_CSV_OK = 0,
    WU_CSV_ERROR_EOF = 1,
    WU_CSV_ERROR_PARSE = 2
} WU_CsvError;

/**
 * WU_Csv_Reader is a concrete implementation of the WU_Reader interface that
 * reads data from a CSV file.
 */
typedef struct WU_Csv_Reader_ {
    struct WU_Reader_ base;
    FILE* file;
    char line_buffer[WU_CSV_MAX_LINE_SIZE];
    bool has_headers;
    WU_DataType data_type;
    WU_CsvError last_error;
    union {
        WU_Candle candle;
        WU_Trade trade;
        WU_Single single_value;
    } data;
}* WU_Csv_Reader;

WU_Csv_Reader wu_csv_reader_new(FILE *file, WU_DataType data_type, bool has_headers);

#endif // WU_READER_H

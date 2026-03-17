# Data Formats

Wu supports multiple data formats for reading market and trading data. The library currently implements CSV and JSON Lines readers.

## CSV Format

CSV files are the most straightforward format. Each line represents a single data point with comma-separated values.

### Candle Data

CSV format for OHLCV candles:

```csv
timestamp,open,high,low,close,volume
1609459200,150.5,151.2,150.1,151.0,1000000
1609462800,151.0,151.8,150.8,151.5,900000
1609466400,151.5,152.1,151.2,151.9,950000
```

### Trade Data

CSV format for individual trades:

```csv
timestamp,price,volume,side
1609459200,150.5,10000,1
1609459201,150.51,5000,0
1609459202,150.49,15000,1
```

Where `side` is 0 for sell, 1 for buy.

### Single Values

CSV format for single-value series (like price or technical indicators):

```csv
timestamp,value
1609459200,150.5
1609462800,151.0
1609466400,151.5
```

## JSON Lines Format

JSON Lines (newline-delimited JSON) provides a flexible, self-documenting format where each line is a valid JSON object. This format is particularly useful when dealing with variable-length data or nested structures.

### Candle Data

JSON Lines format for OHLCV candles:

```json
{"timestamp": 1609459200, "open": 150.5, "high": 151.2, "low": 150.1, "close": 151.0, "volume": 1000000}
{"timestamp": 1609462800, "open": 151.0, "high": 151.8, "low": 150.8, "close": 151.5, "volume": 900000}
{"timestamp": 1609466400, "open": 151.5, "high": 152.1, "low": 151.2, "close": 151.9, "volume": 950000}
```

### Trade Data

JSON Lines format for individual trades:

```json
{"timestamp": 1609459200, "price": 150.5, "volume": 10000, "side": 1}
{"timestamp": 1609459201, "price": 150.51, "volume": 5000, "side": 0}
{"timestamp": 1609459202, "price": 150.49, "volume": 15000, "side": 1}
```

### Single Values

JSON Lines format for single-value series:

```json
{"timestamp": 1609459200, "value": 150.5}
{"timestamp": 1609462800, "value": 151.0}
{"timestamp": 1609466400, "value": 151.5}
```

## Using Different Readers

The reader interface abstracts data format handling. You can switch between CSV and JSON Lines without changing your strategy code.

### CSV Reader

```c
FILE* file = fopen("data.csv", "r");
WU_CsvReader reader = wu_csv_reader_new(
    file,
    WU_DATA_TYPE_CANDLE,
    WU_TIME_UNIT_SECONDS,
    true  // has_headers
);
```

### JSON Reader

```c
FILE* file = fopen("data.jsonl", "r");
WU_JsonReader reader = wu_json_reader_new(
    file,
    WU_DATA_TYPE_CANDLE,
    WU_TIME_UNIT_SECONDS
);
```

The JSON reader expects one valid JSON object per line. Field order doesn't matter, and the format is more tolerant of additional fields in the data.

## Field Requirements

All readers expect the following fields:

- **timestamp**: Unix timestamp (required)
- **open**, **high**, **low**, **close**, **volume**: For candles
- **price**, **volume**, **side**: For trades
- **value**: For single values

## Handling Timestamps

Timestamps are expected to be Unix epoch values (seconds since 1970-01-01 00:00:00 UTC). When reading data, you specify the time unit to help the library correctly interpret the timestamps:

- `WU_TIME_UNIT_SECONDS`: Data is in seconds (default)
- `WU_TIME_UNIT_MINUTES`: Data is in minutes
- `WU_TIME_UNIT_HOURS`: Data is in hours
- `WU_TIME_UNIT_DAYS`: Data is in days

For example, if your CSV contains timestamps in milliseconds, divide them by 1000 before storage, or specify the appropriate time unit when creating the reader.

## Error Handling

Both readers track parsing errors via the `last_error` member:

- `WU_CSV_OK` / `WU_JSON_OK`: Successful read
- `WU_CSV_ERROR_EOF` / `WU_JSON_ERROR_EOF`: End of file reached
- `WU_CSV_ERROR_PARSE` / `WU_JSON_ERROR_PARSE`: Parse error (invalid format or missing required fields)
- `WU_JSON_ERROR_MISSING_FIELD`: Required field not found in JSON object

Check `wu_reader_last_error(reader)` after calling `wu_reader_next()` to detect errors.

## JSON Reader Implementation Details

The JSON reader parses newline-delimited JSON objects. It expects one valid JSON object per line, with field order being irrelevant. The reader is flexible with additional fields—only required fields for the data type must be present. This makes it suitable for data sources that may include extra metadata or fields you don't need.

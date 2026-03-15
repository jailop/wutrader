%module wu

%{
#include "../include/wu.h"
%}

%include <stdint.i>
%include <carrays.i>

%array_functions(double, doubleArray);

/* Typemaps must be defined BEFORE the headers that use them */

/* Typemap to convert Python list of strings to WU_AssetSymbol array */
%typemap(in) (const WU_AssetSymbol *symbols, int num_assets) {
    if (!PyList_Check($input)) {
        PyErr_SetString(PyExc_TypeError, "Expected a list of strings");
        SWIG_fail;
    }
    $2 = PyList_Size($input);
    $1 = (WU_AssetSymbol *)malloc($2 * sizeof(WU_AssetSymbol));
    if ($1 == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory for symbols");
        SWIG_fail;
    }
    for (int i = 0; i < $2; i++) {
        PyObject *item = PyList_GetItem($input, i);
        if (!PyUnicode_Check(item)) {
            free($1);
            PyErr_SetString(PyExc_TypeError, "List items must be strings");
            SWIG_fail;
        }
        const char *str = PyUnicode_AsUTF8(item);
        if (str == NULL) {
            free($1);
            SWIG_fail;
        }
        strncpy($1[i], str, WU_SYMBOL_MAX_LEN - 1);
        $1[i][WU_SYMBOL_MAX_LEN - 1] = '\0';
    }
}

%typemap(freearg) (const WU_AssetSymbol *symbols, int num_assets) {
    if ($1) free($1);
}

/* Typemap to allow BasicPortfolio to be passed as Portfolio */
%typemap(in) WU_Portfolio {
    void *argp = 0;
    int res = SWIG_ConvertPtr($input, &argp, $descriptor(struct WU_BasicPortfolio_ *), 0);
    if (!SWIG_IsOK(res)) {
        res = SWIG_ConvertPtr($input, &argp, $descriptor(WU_Portfolio), 0);
        if (!SWIG_IsOK(res)) {
            SWIG_exception_fail(SWIG_ArgError(res), "Expected Portfolio or BasicPortfolio");
        }
    }
    $1 = (WU_Portfolio)argp;
}

/* Typemap to allow CrossOverStrat to be passed as Strategy */
%typemap(in) WU_Strategy {
    void *argp = 0;
    int res = SWIG_ConvertPtr($input, &argp, $descriptor(struct WU_CrossOverStrat_ *), 0);
    if (!SWIG_IsOK(res)) {
        res = SWIG_ConvertPtr($input, &argp, $descriptor(WU_Strategy), 0);
        if (!SWIG_IsOK(res)) {
            SWIG_exception_fail(SWIG_ArgError(res), "Expected WU_Strategy or WU_CrossOverStrat");
        }
    }
    $1 = (WU_Strategy)argp;
}

/* Typemap to allow WU_CsvReader to be passed as Reader */
%typemap(in) WU_Reader {
    void *argp = 0;
    int res = SWIG_ConvertPtr($input, &argp, $descriptor(struct WU_CsvReader_ *), 0);
    if (!SWIG_IsOK(res)) {
        res = SWIG_ConvertPtr($input, &argp, $descriptor(WU_Reader), 0);
        if (!SWIG_IsOK(res)) {
            SWIG_exception_fail(SWIG_ArgError(res), "Expected WU_Reader or WU_CsvReader");
        }
    }
    $1 = (WU_Reader)argp;
}

/* Include all header files for SWIG to process */
%include "../include/wu/types.h"
%include "../include/wu/data.h"
%include "../include/wu/indicators.h"
%include "../include/wu/readers.h"
%include "../include/wu/portfolios.h"
%include "../include/wu/strategies.h"

/* Ignore the run function pointer field in BasicRunner to avoid conflicts */
%ignore BasicRunner_::run;

/* Ignore the run function pointer field in BasicRunner to avoid conflicts */
%ignore BasicRunner_::run;

%include "../include/wu/runners.h"

%inline %{
/* Helper to open CSV file from filename */
WU_CsvReader wu_csv_reader_open(const char* filename, WU_DataType data_type, bool has_headers) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;
    }
    return wu_csv_reader_new(file, data_type, has_headers);
}
%}

%extend WU_BasicPortfolio_ {
    double get_cash() {
        return $self->cash;
    }
    
    double get_accum_tx_fees() {
        return $self->stats->accum_tx_fees;
    }
    
    double get_accum_borrow_interest() {
        return $self->stats->accum_borrow_interest;
    }
    
    int64_t get_total_trades() {
        return $self->stats->total_trades;
    }
    
    int64_t get_winning_trades() {
        return $self->stats->winning_trades;
    }
    
    int64_t get_losing_trades() {
        return $self->stats->losing_trades;
    }
    
    double get_total_profit() {
        return $self->stats->total_profit;
    }
    
    double get_total_loss() {
        return $self->stats->total_loss;
    }
    
    char* get_keyvalue_stats() {
        if (!$self->stats || !$self->stats->to_keyvalue) return NULL;
        return $self->stats->to_keyvalue($self->stats);
    }
    
    char* get_json_stats(bool pretty) {
        if (!$self->stats || !$self->stats->to_json) return NULL;
        return $self->stats->to_json($self->stats, pretty);
    }
}
    
    void update(WU_Signal* signals) {
        $self->base.update((WU_Portfolio)$self, signals);
    }
    
    double value() {
        return $self->base.value((WU_Portfolio)$self);
    }
    
    double pnl() {
        return $self->base.pnl((WU_Portfolio)$self);
    }
    
    ~WU_BasicPortfolio_() {
        if ($self->base.delete)
            $self->base.delete((WU_Portfolio)$self);
    }
}

%extend CrossOverStrat_ {
    Signal update(const void* data) {
        return $self->base.update((Strategy)$self, data);
    }
    
    ~CrossOverStrat_() {
        if ($self->base.delete)
            $self->base.delete((Strategy)$self);
    }
}

%extend WU_CsvReader_ {
    void* next() {
        return $self->base.next((WU_Reader)$self);
    }
    
    WU_CsvError get_last_error() {
        return $self->last_error;
    }
    
    WU_Candle* read_candle() {
        if ($self->data_type == WU_DATA_TYPE_CANDLE) {
            void* data = $self->base.next((WU_Reader)$self);
            return data ? (WU_Candle*)data : NULL;
        }
        return NULL;
    }
    
    WU_Trade* read_trade() {
        if ($self->data_type == WU_DATA_TYPE_TRADE) {
            void* data = $self->base.next((WU_Reader)$self);
            return data ? (WU_Trade*)data : NULL;
        }
        return NULL;
    }
    
    WU_Single* read_single_value() {
        if ($self->data_type == WU_DATA_TYPE_SINGLE_VALUE) {
            void* data = $self->base.next((WU_Reader)$self);
            return data ? (WU_Single*)data : NULL;
        }
        return NULL;
    }
    
    ~WU_CsvReader_() {
        if ($self->base.delete)
            $self->base.delete((WU_Reader)$self);
    }
}

%extend BasicRunner_ {
    void execute(bool verbose) {
        $self->run($self, verbose);
    }
    
    ~BasicRunner_() {
        basic_runner_free($self);
    }
}

%extend WU_Runner_ {
    void execute(bool verbose) {
        $self->run($self, verbose);
    }
    
    ~WU_Runner_() {
        wu_runner_free($self);
    }
}


%extend PositionVector {
    ~PositionVector() {
        if ($self->delete)
            $self->delete($self);
    }
}

%extend PortfolioStats_ {
    ~PortfolioStats_() {
        portfolio_stats_delete($self);
    }
}

%pythoncode %{
# Version information
__version__ = '0.1.0'

def create_signal(timestamp=0, side=WU_SIDE_HOLD, price=0.0, quantity=0.0):
    """
    Create a Signal with the given parameters.
    
    Args:
        timestamp: Unix timestamp (default: 0)
        side: Signal side - WU_SIDE_BUY, WU_SIDE_SELL, or WU_SIDE_HOLD (default: WU_SIDE_HOLD)
        price: Execution price (default: 0.0)
        quantity: Quantity to trade (default: 0.0)
    
    Returns:
        Signal instance
    """
    return signal_init(timestamp, side, price, quantity)

def create_basic_portfolio(initial_cash=10000.0, tx_cost_pct=0.001, 
                          stop_loss_pct=0.0, take_profit_pct=0.0,
                          slippage_pct=0.0, size_type=WU_POSITION_SIZE_ABS,
                          size_value=1.0, symbols=["ASSET"]):
    """
    Create a BasicPortfolio with the given parameters.
    
    Args:
        initial_cash: Initial cash amount (default: 10000.0)
        tx_cost_pct: Transaction cost percentage (default: 0.001)
        stop_loss_pct: Stop loss percentage (default: 0.0)
        take_profit_pct: Take profit percentage (default: 0.0)
        slippage_pct: Slippage percentage (default: 0.0)
        size_type: Position size type (default: WU_POSITION_SIZE_ABS)
            - WU_POSITION_SIZE_ABS: Absolute quantity (e.g., 100 shares)
            - WU_POSITION_SIZE_PCT: Percentage of available cash (e.g., 0.5 = 50%)
            - WU_POSITION_SIZE_PCT_EQUAL: Equal distribution among all assets
            - WU_POSITION_SIZE_STRATEGY_GUIDED: Strategy specifies target proportion via signal.quantity
        size_value: Position size value (default: 1.0)
            - For ABS: quantity to buy
            - For PCT: percentage (0.0 to 1.0)
            - For PCT_EQUAL: allocation multiplier (0.0 to 1.0)
            - For STRATEGY_GUIDED: allocation multiplier (0.0 to 1.0)
        symbols: List of asset symbols (default: ["ASSET"])
    
    Returns:
        BasicPortfolio instance
    
    Example:
        # Equal distribution among 3 assets
        portfolio = create_basic_portfolio(
            initial_cash=100000.0,
            size_type=WU_POSITION_SIZE_PCT_EQUAL,
            size_value=0.95,
            symbols=["SPY", "QQQ", "TLT"]
        )
        
        # Strategy-guided allocation
        portfolio = create_basic_portfolio(
            initial_cash=100000.0,
            size_type=WU_POSITION_SIZE_STRATEGY_GUIDED,
            size_value=1.0,
            symbols=["SPY", "QQQ", "TLT"]
        )
        # Strategy sets allocations via signal.quantity:
        signals = [
            create_signal(timestamp=1000, side=WU_SIDE_BUY, price=100.0, quantity=0.5),  # 50% SPY
            create_signal(timestamp=1000, side=WU_SIDE_BUY, price=50.0, quantity=0.3),   # 30% QQQ
            create_signal(timestamp=1000, side=WU_SIDE_BUY, price=100.0, quantity=0.2),  # 20% TLT
        ]
    """
    params = WU_PortfolioParams()
    params.initial_cash = initial_cash
    params.tx_cost_pct = tx_cost_pct
    params.stop_loss_pct = stop_loss_pct
    params.take_profit_pct = take_profit_pct
    params.slippage_pct = slippage_pct
    params.position_sizing.size_type = size_type
    params.position_sizing.size_value = size_value
    
    # typemap handles conversion: symbols list -> (WU_AssetSymbol*, int)
    return wu_basic_portfolio_new(params, symbols)

def open_csv_reader(filename, data_type=WU_DATA_TYPE_CANDLE, has_headers=True):
    """
    Open a CSV file for reading market data.
    
    Args:
        filename: Path to CSV file
        data_type: Type of data (WU_DATA_TYPE_CANDLE, WU_DATA_TYPE_TRADE, or WU_DATA_TYPE_SINGLE_VALUE)
        has_headers: Whether the CSV has headers (default: True)
    
    Returns:
        WU_CsvReader instance or None on error
    """
    return wu_csv_reader_open(filename, data_type, has_headers)
%}



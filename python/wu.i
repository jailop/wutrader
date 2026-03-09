%module wu

%{
#include "../include/wu.h"
%}

%include <stdint.i>
%include <carrays.i>

%array_functions(double, doubleArray);

/* Typemaps must be defined BEFORE the headers that use them */

/* Typemap to allow MovingAverage to be passed as Indicator */
%typemap(in) Indicator {
    void *argp = 0;
    int res = SWIG_ConvertPtr($input, &argp, $descriptor(struct MovingAverage_ *), 0);
    if (!SWIG_IsOK(res)) {
        res = SWIG_ConvertPtr($input, &argp, $descriptor(struct ExponentialMovingAverage_ *), 0);
        if (!SWIG_IsOK(res)) {
            res = SWIG_ConvertPtr($input, &argp, $descriptor(Indicator), 0);
            if (!SWIG_IsOK(res)) {
                SWIG_exception_fail(SWIG_ArgError(res), "Expected Indicator, MovingAverage, or ExponentialMovingAverage");
            }
        }
    }
    $1 = (Indicator)argp;
}

/* Typemap to allow SingleAssetPortfolio to be passed as Portfolio */
%typemap(in) Portfolio {
    void *argp = 0;
    int res = SWIG_ConvertPtr($input, &argp, $descriptor(struct SingleAssetPortfolio_ *), 0);
    if (!SWIG_IsOK(res)) {
        res = SWIG_ConvertPtr($input, &argp, $descriptor(Portfolio), 0);
        if (!SWIG_IsOK(res)) {
            SWIG_exception_fail(SWIG_ArgError(res), "Expected Portfolio or SingleAssetPortfolio");
        }
    }
    $1 = (Portfolio)argp;
}

/* Typemap to allow CrossOverStrat to be passed as Strategy */
%typemap(in) Strategy {
    void *argp = 0;
    int res = SWIG_ConvertPtr($input, &argp, $descriptor(struct CrossOverStrat_ *), 0);
    if (!SWIG_IsOK(res)) {
        res = SWIG_ConvertPtr($input, &argp, $descriptor(Strategy), 0);
        if (!SWIG_IsOK(res)) {
            SWIG_exception_fail(SWIG_ArgError(res), "Expected Strategy or CrossOverStrat");
        }
    }
    $1 = (Strategy)argp;
}

/* Typemap to allow CsvReader to be passed as Reader */
%typemap(in) Reader {
    void *argp = 0;
    int res = SWIG_ConvertPtr($input, &argp, $descriptor(struct CsvReader_ *), 0);
    if (!SWIG_IsOK(res)) {
        res = SWIG_ConvertPtr($input, &argp, $descriptor(Reader), 0);
        if (!SWIG_IsOK(res)) {
            SWIG_exception_fail(SWIG_ArgError(res), "Expected Reader or CsvReader");
        }
    }
    $1 = (Reader)argp;
}

/* Include all header files for SWIG to process */
%include "../include/wu/types.h"
%include "../include/wu/data.h"
%include "../include/wu/indicator.h"
%include "../include/wu/reader.h"
%include "../include/wu/portfolio.h"
%include "../include/wu/strategy.h"

/* Ignore the run function pointer field in BasicRunner to avoid conflicts */
%ignore BasicRunner_::run;

/* Ignore the run function pointer field in BasicRunner to avoid conflicts */
%ignore BasicRunner_::run;

%include "../include/wu/runner.h"

%inline %{
/* Indicator interface - wraps the C macros for Python use */
void wu_indicator_update(Indicator ind, double value) {
    if (ind && ind->update) {
        ind->update(ind, &value);
    }
}

double wu_indicator_value(Indicator ind) {
    if (ind && ind->value) {
        double* val_ptr = (double*)ind->value(ind);
        return val_ptr ? *val_ptr : 0.0;
    }
    return 0.0;
}

Signal strategy_call_update(Strategy strategy, void* data) {
    return strategy->update(strategy, data);
}

void* reader_call_next(Reader reader) {
    return reader->next(reader);
}

void portfolio_call_update(Portfolio portfolio, Signal signal) {
    portfolio->update(portfolio, signal);
}

double portfolio_call_value(Portfolio portfolio) {
    return portfolio->value(portfolio);
}

double portfolio_call_pnl(Portfolio portfolio) {
    return portfolio->pnl(portfolio);
}

void runner_call_run(BasicRunner runner, bool verbose) {
    runner->run(runner, verbose);
}

/* Helper to open CSV file from filename */
CsvReader csv_reader_open(const char* filename, DataType data_type, bool has_headers) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;
    }
    return csv_reader_new(file, data_type, has_headers);
}
%}

%extend SingleAssetPortfolio_ {
    double get_cash() {
        return $self->track.cash;
    }
    
    double get_last_price() {
        return $self->track.last_price;
    }
    
    double get_accum_expenses() {
        return $self->track.accum_expenses;
    }
    
    int64_t get_total_trades() {
        return $self->track.stats->total_trades;
    }
    
    int64_t get_winning_trades() {
        return $self->track.stats->winning_trades;
    }
    
    int64_t get_losing_trades() {
        return $self->track.stats->losing_trades;
    }
    
    double get_total_profit() {
        return $self->track.stats->total_profit;
    }
    
    double get_total_loss() {
        return $self->track.stats->total_loss;
    }
    
    void update(Signal signal) {
        $self->base.update((Portfolio)$self, signal);
    }
    
    double value() {
        return $self->base.value((Portfolio)$self);
    }
    
    double pnl() {
        return $self->base.pnl((Portfolio)$self);
    }
    
    ~SingleAssetPortfolio_() {
        if ($self->base.delete)
            $self->base.delete((Portfolio)$self);
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

%extend CsvReader_ {
    void* next() {
        return $self->base.next((Reader)$self);
    }
    
    CsvError get_last_error() {
        return $self->last_error;
    }
    
    Candle* read_candle() {
        if ($self->data_type == DATA_TYPE_CANDLE) {
            void* data = $self->base.next((Reader)$self);
            return data ? (Candle*)data : NULL;
        }
        return NULL;
    }
    
    Trade* read_trade() {
        if ($self->data_type == DATA_TYPE_TRADE) {
            void* data = $self->base.next((Reader)$self);
            return data ? (Trade*)data : NULL;
        }
        return NULL;
    }
    
    SingleValue* read_single_value() {
        if ($self->data_type == DATA_TYPE_SINGLE_VALUE) {
            void* data = $self->base.next((Reader)$self);
            return data ? (SingleValue*)data : NULL;
        }
        return NULL;
    }
    
    ~CsvReader_() {
        if ($self->base.delete)
            $self->base.delete((Reader)$self);
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

%extend MovingAverage_ {
    void update(double value) {
        if ($self->base.update) {
            $self->base.update((Indicator)$self, &value);
        }
    }
    
    double value() {
        if ($self->base.value) {
            double* val_ptr = (double*)$self->base.value((Indicator)$self);
            return val_ptr ? *val_ptr : 0.0;
        }
        return 0.0;
    }
    
    ~MovingAverage_() {
        if ($self->base.delete)
            $self->base.delete((Indicator)$self);
    }
}

%extend ExponentialMovingAverage_ {
    void update(double value) {
        if ($self->base.update) {
            $self->base.update((Indicator)$self, &value);
        }
    }
    
    double value() {
        if ($self->base.value) {
            double* val_ptr = (double*)$self->base.value((Indicator)$self);
            return val_ptr ? *val_ptr : 0.0;
        }
        return 0.0;
    }
    
    ~ExponentialMovingAverage_() {
        if ($self->base.delete)
            $self->base.delete((Indicator)$self);
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

# Alias to match C macro names
indicator_update = wu_indicator_update
indicator_value = wu_indicator_value

def create_signal(timestamp=0, side=SIDE_HOLD, price=0.0, quantity=0.0):
    """
    Create a Signal with the given parameters.
    
    Args:
        timestamp: Unix timestamp (default: 0)
        side: Signal side - SIDE_BUY, SIDE_SELL, or SIDE_HOLD (default: SIDE_HOLD)
        price: Execution price (default: 0.0)
        quantity: Quantity to trade (default: 0.0)
    
    Returns:
        Signal instance
    """
    return signal_init(timestamp, side, price, quantity)

def create_single_asset_portfolio(initial_cash=10000.0, tx_cost_pct=0.001, 
                                  stop_loss_pct=0.0, take_profit_pct=0.0,
                                  slippage_pct=0.0, size_type=POSITION_SIZE_ABS,
                                  size_value=1.0):
    """
    Create a SingleAssetPortfolio with the given parameters.
    
    Args:
        initial_cash: Initial cash amount (default: 10000.0)
        tx_cost_pct: Transaction cost percentage (default: 0.001)
        stop_loss_pct: Stop loss percentage (default: 0.0)
        take_profit_pct: Take profit percentage (default: 0.0)
        slippage_pct: Slippage percentage (default: 0.0)
        size_type: Position size type (POSITION_SIZE_ABS or POSITION_SIZE_PCT, default: POSITION_SIZE_ABS)
        size_value: Position size value (default: 1.0)
    
    Returns:
        SingleAssetPortfolio instance
    """
    params = SingleAssetPortfolioParams()
    params.initial_cash = initial_cash
    params.tx_cost_pct = tx_cost_pct
    params.stop_loss_pct = stop_loss_pct
    params.take_profit_pct = take_profit_pct
    params.slippage_pct = slippage_pct
    params.position_sizing.size_type = size_type
    params.position_sizing.size_value = size_value
    
    return single_asset_portfolio_new(params)

def open_csv_reader(filename, data_type=DATA_TYPE_CANDLE, has_headers=True):
    """
    Open a CSV file for reading market data.
    
    Args:
        filename: Path to CSV file
        data_type: Type of data (DATA_TYPE_CANDLE, DATA_TYPE_TRADE, or DATA_TYPE_SINGLE_VALUE)
        has_headers: Whether the CSV has headers (default: True)
    
    Returns:
        CsvReader instance or None on error
    """
    return csv_reader_open(filename, data_type, has_headers)

# Enable automatic memory management
# Objects will be automatically freed when garbage collected
%}

%feature("python:annotations", "c") SingleAssetPortfolio_;
%feature("python:annotations", "c") CrossOverStrat_;
%feature("python:annotations", "c") CsvReader_;
%feature("python:annotations", "c") BasicRunner_;
%feature("python:annotations", "c") MovingAverage_;
%feature("python:annotations", "c") ExponentialMovingAverage_;
%feature("python:annotations", "c") PositionVector;
%feature("python:annotations", "c") PortfolioStats_;

%module wu

%{
#include "../include/wu.h"
%}

%include <stdint.i>
%include <carrays.i>

%array_functions(double, doubleArray);

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

%include "../include/wu.h"

%inline %{
/* Indicator interface - wraps the C macros for Python use */
void wu_indicator_update(Indicator ind, double value) {
    if (ind && ind->update) {
        ind->update(ind, value);
    }
}

double wu_indicator_value(Indicator ind) {
    return ind ? ind->value : 0.0;
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
    
    ~SingleAssetPortfolio_() {
        single_asset_portfolio_free($self);
    }
}

%extend CrossOverStrat_ {
    ~CrossOverStrat_() {
        cross_over_strat_free($self);
    }
}

%extend CsvReader_ {
    ~CsvReader_() {
        csv_reader_free($self);
    }
}

%extend BasicRunner_ {
    ~BasicRunner_() {
        basic_runner_free($self);
    }
}

%extend MovingAverage_ {
    ~MovingAverage_() {
        moving_average_free($self);
    }
}

%extend ExponentialMovingAverage_ {
    ~ExponentialMovingAverage_() {
        exponential_moving_average_free($self);
    }
}

%extend PositionVector {
    ~PositionVector() {
        position_vector_free($self);
    }
}

%extend PortfolioStats_ {
    ~PortfolioStats_() {
        portfolio_stats_free($self);
    }
}

%extend CsvReader_ {
    CsvError get_last_error() {
        return $self->last_error;
    }
}

%pythoncode %{
# Alias to match C macro names
indicator_update = wu_indicator_update
indicator_value = wu_indicator_value

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

# Execution Models

Wu's portfolio implementation models how trades get executed. The current
implementation makes several simplifying assumptions that affect backtest
realism.

## Market Orders Only

At this moment, Wu supports only market orders—orders that execute
immediately at the current market price. The library does not implement
an order book, so other order types like limit orders, stop orders, or
conditional orders are not supported.

**Why market orders only?** Implementing an order book requires modeling
order matching, price-time priority, queue dynamics, and partial fills.
This adds significant complexity for uncertain benefit in backtesting
context. Most historical data doesn't include order book depth anyway.

**What this means**: When your strategy generates a signal, the portfolio
executes it immediately (or on next bar for NEXT_CLOSE policy). You can't
say "buy if price drops to $100" or "sell if price rises to $105". The
strategy must check conditions and generate signals explicitly.

## Execution Policies

The `WU_ExecutionPolicy` enum controls when and how orders execute:

### IMMEDIATE

Orders execute instantly at the signal price. When the strategy says
"buy", the portfolio buys immediately at the price provided in the
signal.

This is the simplest model but somewhat unrealistic. In practice, you see
a signal at the close of a bar and execute on the next bar's open or
close. IMMEDIATE assumes zero latency between signal and execution.

### NEXT_CLOSE

Orders execute at the next bar's close price. When a signal arrives, the
portfolio stores it as pending. On the next update, the portfolio
executes pending orders at the current prices.

This is more realistic for daily or slower timeframe strategies. You
calculate indicators at bar close, generate signals, and execute at the
next bar's close. There's a one-bar delay between signal and execution.

The implementation uses a `pending_orders` array in the portfolio struct.
Each asset can have at most one pending order. If a new signal arrives
before the pending order executes, it overwrites the pending order.

### FIXED_SLIPPAGE

Orders execute immediately with fixed percentage slippage applied. If
`execution_mean = 0.0005` (0.05%), a buy order for $100 executes at
$100.05 and a sell order executes at $99.95.

This models the cost of immediacy—you pay a premium to get filled right
away. The fixed percentage is a simplification. Real slippage depends on
order size, volatility, liquidity, and market conditions.

### RANDOM_SLIPPAGE

Orders execute immediately with random slippage drawn from a
distribution. The `execution_mean` and `execution_stddev` parameters
define the distribution. Slippage is sampled as `mean ± stddev × random`.

This adds variability to model how market impact varies. Sometimes you
get lucky and slip less than average. Sometimes you slip more. Over many
trades, it averages out to the mean, but individual trades vary.

The randomness uses `rand()` from the C standard library. For
reproducible results, seed it with `srand()` before running backtests.

## Transaction Costs

Transaction costs are separate from execution price. They represent
commissions, exchange fees, and other fixed or proportional costs.

**FIXED costs** charge a flat dollar amount per trade. Setting
`tx_cost_type = WU_TRANSACTION_COST_FIXED` with `tx_cost_value = 5.0`
charges $5 per trade regardless of trade size. Useful for modeling
per-trade broker commissions.

**PROPORTIONAL costs** charge a percentage of trade value. Setting
`tx_cost_type = WU_TRANSACTION_COST_PROPORTIONAL` with
`tx_cost_value = 0.001` charges 0.1% of the trade amount. A $10,000 trade
costs $10. Useful for percentage-based fee structures.

Transaction costs are deducted from portfolio cash separately from the
trade amount. A $10,000 buy order with $10 transaction cost reduces cash
by $10,010.

## What's Missing

**No limit orders**: You can't place an order that waits for a price
level. The strategy must poll and generate signals when conditions are
met.

**No order book**: There's no simulation of order matching, queue
position, or price improvement. Orders execute atomically at a single
price.

**No partial fills**: Your entire order fills or none of it fills (if you
lack cash). In reality, large orders might fill partially at multiple
price levels.

**No order cancellation**: Once a signal generates, it either executes
immediately (IMMEDIATE, FIXED/RANDOM_SLIPPAGE) or executes next bar
(NEXT_CLOSE). You can't cancel pending orders explicitly.

**No market impact modeling**: Slippage is proportional to trade value,
not order size in shares. Real market impact is nonlinear—doubling order
size more than doubles price impact.

## Design Rationale

The execution model balances simplicity with usefulness. Market orders
with slippage capture the essential cost of trading without requiring
complex order book simulation. Most backtesting scenarios don't need more
detail than this.

If you need sophisticated execution modeling, implement a custom portfolio
that tracks order books, simulates market making, or models limit order
dynamics. The portfolio interface is generic enough to support this.

The current implementation serves as a reasonable starting point for
exploring strategy behavior. More complex execution can be layered on top
without changing the core abstractions.

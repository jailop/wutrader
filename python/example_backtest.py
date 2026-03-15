#!/usr/bin/env python3
"""
Complete backtesting example using WU Python bindings
Based on examples/backtest/example01.c
"""

import wu
import sys
import os


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <csv_file> [-v]", file=sys.stderr)
        return 1
    filename = sys.argv[1]
    verbose = len(sys.argv) > 2 and sys.argv[2] == "-v"
    if not os.path.exists(filename):
        print(f"Error: Cannot open file {filename}", file=sys.stderr)
        return 1
    initial_cash = 100000.0
    portfolio = wu.create_basic_portfolio(
        initial_cash=initial_cash,
        tx_cost_pct=0.001,
        stop_loss_pct=0.10,
        take_profit_pct=0.20,
        slippage_pct=0.0005,
        size_type=wu.WU_POSITION_SIZE_PCT,
        size_value=1.0,
        symbols=["BTCUSD"],
    )
    strategy = wu.wu_crossover_strat_new(10, 30, 0.0)
    reader = wu.open_csv_reader(filename, wu.WU_DATA_TYPE_CANDLE, True)
    if not portfolio or not strategy or not reader:
        print("Error: Failed to initialize components", file=sys.stderr)
        return 1
    runner = wu.wu_runner_new_single(portfolio, strategy, reader)
    if not runner:
        print("Error: Failed to create runner", file=sys.stderr)
        return 1

    runner.execute(verbose)

    pnl = portfolio.pnl()
    pnl_pct = (pnl / initial_cash) * 100.0

    print(f"Initial Cash:      {initial_cash:.2f}")
    print(f"Final Value:       {portfolio.value():.2f}")
    print(f"P&L:               {pnl:.2f} ({pnl_pct:.2f}%)")
    print(f"Transaction Fees:  {portfolio.get_accum_tx_fees():.2f}")
    print(f"Borrow Interest:   {portfolio.get_accum_borrow_interest():.2f}")
    print(f"Total Trades:      {portfolio.get_total_trades()}")
    print(f"Winning Trades:    {portfolio.get_winning_trades()}")
    print(f"Losing Trades:     {portfolio.get_losing_trades()}")
    total_trades = portfolio.get_total_trades()
    if total_trades > 0:
        win_rate = (portfolio.get_winning_trades() * 100.0) / total_trades
        print(f"Win Rate:          {win_rate:.2f}%")

    print(
        f"Stop Loss Exits:   {portfolio.get_total_trades() - portfolio.get_winning_trades() - portfolio.get_losing_trades()}"
    )
    print(
        f"Take Profit Exits: {portfolio.get_total_trades() - portfolio.get_winning_trades() - portfolio.get_losing_trades()}"
    )
    print(f"Total Profit:      {portfolio.get_total_profit():.2f}")
    print(f"Total Loss:        {portfolio.get_total_loss():.2f}")
    winning = portfolio.get_winning_trades()
    losing = portfolio.get_losing_trades()
    if winning > 0:
        avg_win = portfolio.get_total_profit() / winning
        print(f"Avg Win:           ${avg_win:.2f}")
    if losing > 0:
        avg_loss = portfolio.get_total_loss() / losing
        print(f"Avg Loss:          ${avg_loss:.2f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

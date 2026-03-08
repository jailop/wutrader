#!/usr/bin/env python3
"""
Example demonstrating WU Python bindings

Note: Memory management is automatic - no need to manually free objects
"""

import wu

def main():
    print("WU Python Bindings Example")
    print("=" * 50)
    
    # Test enums
    print("\n1. Testing Enums:")
    print(f"   SIDE_BUY = {wu.SIDE_BUY}")
    print(f"   SIDE_SELL = {wu.SIDE_SELL}")
    print(f"   DATA_TYPE_CANDLE = {wu.DATA_TYPE_CANDLE}")
    
    # Test creating indicators
    print("\n2. Creating Moving Average:")
    ma = wu.moving_average_new(10)
    print(f"   MA created with window size 10")
    
    # Update moving average with some values
    values = [100.0, 101.0, 102.0, 103.0, 104.0]
    for val in values:
        wu.indicator_update(ma, val)
        result = wu.indicator_value(ma)
        print(f"   Updated with {val}, result: {result}")
    
    # No need to call moving_average_free - automatic cleanup
    
    # Test creating a portfolio
    print("\n3. Creating Portfolio:")
    portfolio = wu.create_single_asset_portfolio(
        initial_cash=10000.0,
        tx_cost_pct=0.001,
        stop_loss_pct=0.02,
        take_profit_pct=0.05,
        size_type=wu.POSITION_SIZE_ABS,
        size_value=1.0
    )
    print(f"   Portfolio created")
    print(f"   Initial cash: ${portfolio.get_cash():.2f}")
    
    # Test creating and processing signal
    print("\n4. Creating and Processing Signal:")
    signal = wu.Signal()
    signal.timestamp = 1000000
    signal.side = wu.SIDE_BUY
    signal.price = 100.0
    signal.quantity = 10.0
    print(f"   Signal: {['BUY', 'SELL', 'HOLD'][signal.side]} @ ${signal.price}")
    
    wu.portfolio_call_update(portfolio, signal)
    print(f"   Portfolio value after BUY: ${wu.portfolio_call_value(portfolio):.2f}")
    print(f"   Cash: ${portfolio.get_cash():.2f}")
    
    # Test creating a strategy
    print("\n5. Creating Crossover Strategy:")
    strategy = wu.cross_over_strat_new(
        short_window=5,
        long_window=20,
        threshold=0.001
    )
    print(f"   Crossover strategy created (5/20 windows)")
    
    # Test creating a candle
    print("\n6. Creating Candle Data:")
    candle = wu.candle_init(
        timestamp=1000000,
        open=100.0,
        high=105.0,
        low=99.0,
        close=103.0,
        volume=10000.0
    )
    print(f"   Candle: O={candle.open} H={candle.high} L={candle.low} C={candle.close} V={candle.volume}")
    
    # No cleanup needed - automatic via destructors
    
    print("\n" + "=" * 50)
    print("All tests passed successfully!")

if __name__ == "__main__":
    main()

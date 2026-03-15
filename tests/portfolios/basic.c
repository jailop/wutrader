#include <CUnit/CUnit.h>
#include <string.h>
#include "wu.h"

void test_basic_portfolio_initialization(void) {
    WU_PortfolioParams params = {
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.001,
        .stop_loss_pct = 0.05,
        .take_profit_pct = 0.10,
        .slippage_pct = 0.0005,
        .direction = WU_DIRECTION_LONG,
        .borrow_rate = 0.0,
        .borrow_limit = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 0.45
        }
    };
    
    const char* symbols[] = {"SPY", "QQQ", "TLT", NULL};
    
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    
    CU_ASSERT_PTR_NOT_NULL(portfolio);
    CU_ASSERT_EQUAL(portfolio->num_assets, 3);
    CU_ASSERT_DOUBLE_EQUAL(portfolio->cash, 100000.0, 0.001);
    
    // Verify all positions start at zero
    for (int i = 0; i < 3; i++) {
        double qty = wu_basic_portfolio_asset_quantity(portfolio, i);
        CU_ASSERT_DOUBLE_EQUAL(qty, 0.0, 0.0001);
    }
    
    wu_portfolio_delete((WU_Portfolio)portfolio);
}

void test_basic_portfolio_single_buy_signal(void) {
    WU_PortfolioParams params = {
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.001,
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .direction = WU_DIRECTION_LONG,
        .borrow_rate = 0.0,
        .borrow_limit = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 0.50
        }
    };
    
    const char* symbols[] = {"SPY", "QQQ", NULL};
    
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    
    // Create a BUY signal for SPY at $100
    WU_Signal signals[2];
    signals[0].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    signals[0].side = WU_SIDE_BUY;
    signals[0].price = 100.0;
    signals[0].quantity = 1.0;
    
    signals[1].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    signals[1].side = WU_SIDE_HOLD;
    signals[1].price = 50.0;
    signals[1].quantity = 0.0;
    
    wu_portfolio_update((WU_Portfolio)portfolio, signals);
    
    // Should have bought SPY with 50% of cash = $50,000 / $100 = 500 shares
    double spy_qty = wu_basic_portfolio_asset_quantity(portfolio, 0);
    CU_ASSERT_TRUE(spy_qty > 0.0);
    CU_ASSERT_TRUE(spy_qty < 510.0);  // Less than 510 due to fees
    
    // Cash should be reduced
    CU_ASSERT_TRUE(portfolio->cash < 100000.0);
    CU_ASSERT_TRUE(portfolio->cash > 49000.0);  // More than 49k remaining
    
    wu_portfolio_delete((WU_Portfolio)portfolio);
}

void test_basic_portfolio_multiple_buy_signals(void) {
    WU_PortfolioParams params = {
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.0,  // No fees for simpler math
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .direction = WU_DIRECTION_LONG,
        .borrow_rate = 0.0,
        .borrow_limit = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 0.50  // 50% per position
        }
    };
    
    const char* symbols[] = {"SPY", "QQQ", NULL};
    
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    
    // Create BUY signals for both assets
    WU_Signal signals[2];
    signals[0].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    signals[0].side = WU_SIDE_BUY;
    signals[0].price = 100.0;
    signals[0].quantity = 1.0;
    
    signals[1].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    signals[1].side = WU_SIDE_BUY;
    signals[1].price = 50.0;
    signals[1].quantity = 1.0;
    
    wu_portfolio_update((WU_Portfolio)portfolio, signals);
    
    // With 2 BUY signals, 50% sizing, and fair cash allocation:
    // Each signal gets cash/2 = $50k
    // Then 50% sizing is applied: 50% of $50k = $25k per signal
    // SPY: $25,000 / $100 = 250 shares
    // QQQ: $25,000 / $50 = 500 shares
    
    double spy_qty = wu_basic_portfolio_asset_quantity(portfolio, 0);
    double qqq_qty = wu_basic_portfolio_asset_quantity(portfolio, 1);
    
    CU_ASSERT_TRUE(spy_qty > 240.0 && spy_qty < 260.0);
    CU_ASSERT_TRUE(qqq_qty > 490.0 && qqq_qty < 510.0);
    
    // Cash should have $50k remaining (50% of initial cash)
    CU_ASSERT_TRUE(portfolio->cash > 49000.0 && portfolio->cash < 51000.0);
    
    wu_portfolio_delete((WU_Portfolio)portfolio);
}

void test_basic_portfolio_sell_before_buy(void) {
    WU_PortfolioParams params = {
        .initial_cash = 50000.0,
        .tx_cost_pct = 0.0,
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .direction = WU_DIRECTION_LONG,
        .borrow_rate = 0.0,
        .borrow_limit = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 1.0
        }
    };
    
    const char* symbols[] = {"SPY", "QQQ", NULL};
    
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    
    // First, buy SPY
    WU_Signal buy_signals[2];
    buy_signals[0].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    buy_signals[0].side = WU_SIDE_BUY;
    buy_signals[0].price = 100.0;
    buy_signals[0].quantity = 1.0;
    buy_signals[1].side = WU_SIDE_HOLD;
    
    wu_portfolio_update((WU_Portfolio)portfolio, buy_signals);
    
    double spy_qty = wu_basic_portfolio_asset_quantity(portfolio, 0);
    CU_ASSERT_TRUE(spy_qty > 0.0);
    
    // Now SELL SPY and BUY QQQ in same update
    WU_Signal mixed_signals[2];
    mixed_signals[0].timestamp = (WU_TimeStamp){.mark = 2000, .units = WU_TIME_UNIT_SECONDS};
    mixed_signals[0].side = WU_SIDE_SELL;
    mixed_signals[0].price = 110.0;  // Sold at profit
    mixed_signals[0].quantity = spy_qty;
    
    mixed_signals[1].timestamp = (WU_TimeStamp){.mark = 2000, .units = WU_TIME_UNIT_SECONDS};
    mixed_signals[1].side = WU_SIDE_BUY;
    mixed_signals[1].price = 50.0;
    mixed_signals[1].quantity = 1.0;
    
    wu_portfolio_update((WU_Portfolio)portfolio, mixed_signals);
    
    // SPY should be sold (qty = 0)
    spy_qty = wu_basic_portfolio_asset_quantity(portfolio, 0);
    CU_ASSERT_DOUBLE_EQUAL(spy_qty, 0.0, 0.0001);
    
    // QQQ should be bought
    double qqq_qty = wu_basic_portfolio_asset_quantity(portfolio, 1);
    CU_ASSERT_TRUE(qqq_qty > 0.0);
    
    // Cash should reflect: initial - SPY_cost + SPY_proceeds - QQQ_cost
    CU_ASSERT_TRUE(portfolio->cash >= 0.0);
    
    wu_portfolio_delete((WU_Portfolio)portfolio);
}

void test_basic_portfolio_asset_value(void) {
    WU_PortfolioParams params = {
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.0,
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .direction = WU_DIRECTION_LONG,
        .borrow_rate = 0.0,
        .borrow_limit = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 1.0
        }
    };
    
    const char* symbols[] = {"SPY", "QQQ", NULL};
    
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    
    // Buy SPY at $100
    WU_Signal signals[2];
    signals[0].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    signals[0].side = WU_SIDE_BUY;
    signals[0].price = 100.0;
    signals[0].quantity = 1.0;
    signals[1].side = WU_SIDE_HOLD;
    
    wu_portfolio_update((WU_Portfolio)portfolio, signals);
    
    double spy_qty = wu_basic_portfolio_asset_quantity(portfolio, 0);
    
    // Update price to $110 via HOLD signal
    signals[0].timestamp = (WU_TimeStamp){.mark = 2000, .units = WU_TIME_UNIT_SECONDS};
    signals[0].side = WU_SIDE_HOLD;
    signals[0].price = 110.0;
    
    wu_portfolio_update((WU_Portfolio)portfolio, signals);
    
    // Asset value should be qty * new_price
    double spy_value = wu_basic_portfolio_asset_value(portfolio, 0);
    double expected_value = spy_qty * 110.0;
    CU_ASSERT_DOUBLE_EQUAL(spy_value, expected_value, 0.01);
    
    wu_portfolio_delete((WU_Portfolio)portfolio);
}

void test_basic_portfolio_total_value(void) {
    WU_PortfolioParams params = {
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.0,
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .direction = WU_DIRECTION_LONG,
        .borrow_rate = 0.0,
        .borrow_limit = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 0.50
        }
    };
    
    const char* symbols[] = {"SPY", "QQQ", NULL};
    
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    
    // Buy both assets
    WU_Signal signals[2];
    signals[0].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    signals[0].side = WU_SIDE_BUY;
    signals[0].price = 100.0;
    signals[0].quantity = 1.0;
    
    signals[1].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    signals[1].side = WU_SIDE_BUY;
    signals[1].price = 50.0;
    signals[1].quantity = 1.0;
    
    wu_portfolio_update((WU_Portfolio)portfolio, signals);
    
    // Total value should equal cash + all asset values
    double total_value = wu_portfolio_value((WU_Portfolio)portfolio);
    CU_ASSERT_TRUE(total_value >= 99900.0 && total_value <= 100100.0);
    
    wu_portfolio_delete((WU_Portfolio)portfolio);
}

void test_basic_portfolio_equal_distribution_sizing(void) {
    // Test WU_POSITION_SIZE_PCT_EQUAL position sizing policy
    // This policy divides total portfolio value equally among all assets
    WU_PortfolioParams params = {
        .initial_cash = 90000.0,  // $90k initial cash
        .tx_cost_pct = 0.0,       // No fees for simpler math
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .direction = WU_DIRECTION_LONG,
        .borrow_rate = 0.0,
        .borrow_limit = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT_EQUAL,
            .size_value = 1.0     // Use 100% of target allocation
        }
    };
    
    // Create 3-asset portfolio
    const char* symbols[] = {"SPY", "QQQ", "TLT", NULL};
    
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    
    // Initial state: $90k cash, 0 positions
    // Each asset should target: $90k / 3 = $30k
    
    // BUY SPY at $100/share -> should buy ~300 shares ($30k worth)
    WU_Signal signals[3];
    signals[0].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    signals[0].side = WU_SIDE_BUY;
    signals[0].price = 100.0;
    signals[0].quantity = 1.0;
    
    signals[1].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    signals[1].side = WU_SIDE_HOLD;
    signals[1].price = 50.0;
    signals[1].quantity = 0.0;
    
    signals[2].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    signals[2].side = WU_SIDE_HOLD;
    signals[2].price = 100.0;
    signals[2].quantity = 0.0;
    
    wu_portfolio_update((WU_Portfolio)portfolio, signals);
    
    // Verify SPY position: should have ~300 shares ($30k / $100)
    double spy_qty = wu_basic_portfolio_asset_quantity(portfolio, 0);
    CU_ASSERT_DOUBLE_EQUAL(spy_qty, 300.0, 1.0);  // Allow 1 share tolerance
    
    // Verify cash: should have ~$60k left ($90k - $30k)
    CU_ASSERT_DOUBLE_EQUAL(portfolio->cash, 60000.0, 100.0);
    
    // Now BUY both QQQ and TLT simultaneously
    // Portfolio value = $60k cash + $30k SPY = $90k
    // Each asset targets: $90k / 3 = $30k
    // SPY already has $30k, so needs 0
    // QQQ should target $30k at $50/share -> 600 shares
    // TLT should target $30k at $100/share -> 300 shares
    // Total needed: $60k, which is exactly what we have in cash
    
    signals[0].timestamp = (WU_TimeStamp){.mark = 2000, .units = WU_TIME_UNIT_SECONDS};
    signals[0].side = WU_SIDE_HOLD;
    signals[0].price = 100.0;  // SPY still at $100
    
    signals[1].timestamp = (WU_TimeStamp){.mark = 2000, .units = WU_TIME_UNIT_SECONDS};
    signals[1].side = WU_SIDE_BUY;
    signals[1].price = 50.0;   // QQQ at $50
    
    signals[2].timestamp = (WU_TimeStamp){.mark = 2000, .units = WU_TIME_UNIT_SECONDS};
    signals[2].side = WU_SIDE_BUY;
    signals[2].price = 100.0;  // TLT at $100
    
    wu_portfolio_update((WU_Portfolio)portfolio, signals);
    
    // Verify all positions are balanced by checking their values
    // Each position should be worth ~$30k
    double spy_value = wu_basic_portfolio_asset_value(portfolio, 0);
    double qqq_value = wu_basic_portfolio_asset_value(portfolio, 1);
    double tlt_value = wu_basic_portfolio_asset_value(portfolio, 2);
    
    CU_ASSERT_DOUBLE_EQUAL(spy_value, 30000.0, 500.0);  // ~$30k
    CU_ASSERT_DOUBLE_EQUAL(qqq_value, 30000.0, 500.0);  // ~$30k
    CU_ASSERT_DOUBLE_EQUAL(tlt_value, 30000.0, 500.0);  // ~$30k
    
    // Total portfolio value should still be ~$90k
    double total_value = wu_portfolio_value((WU_Portfolio)portfolio);
    CU_ASSERT_DOUBLE_EQUAL(total_value, 90000.0, 1000.0);
    
    // Test with size_value = 0.5 (use only 50% of target allocation)
    wu_portfolio_delete((WU_Portfolio)portfolio);
    
    params.position_sizing.size_value = 0.5;  // Only 50% of target
    portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    
    // With 3 assets and 50% allocation, each asset targets:
    // ($90k / 3) * 0.5 = $15k
    signals[0].timestamp = (WU_TimeStamp){.mark = 3000, .units = WU_TIME_UNIT_SECONDS};
    signals[0].side = WU_SIDE_BUY;
    signals[0].price = 100.0;
    
    signals[1].timestamp = (WU_TimeStamp){.mark = 3000, .units = WU_TIME_UNIT_SECONDS};
    signals[1].side = WU_SIDE_HOLD;
    signals[1].price = 50.0;
    
    signals[2].timestamp = (WU_TimeStamp){.mark = 3000, .units = WU_TIME_UNIT_SECONDS};
    signals[2].side = WU_SIDE_HOLD;
    signals[2].price = 100.0;
    
    wu_portfolio_update((WU_Portfolio)portfolio, signals);
    
    // SPY should have ~150 shares ($15k / $100)
    spy_qty = wu_basic_portfolio_asset_quantity(portfolio, 0);
    CU_ASSERT_DOUBLE_EQUAL(spy_qty, 150.0, 1.0);
    
    // Cash should be ~$75k ($90k - $15k)
    CU_ASSERT_DOUBLE_EQUAL(portfolio->cash, 75000.0, 100.0);
    
    wu_portfolio_delete((WU_Portfolio)portfolio);
}

void test_basic_portfolio_strategy_guided_sizing(void) {
    // Test WU_POSITION_SIZE_PCT position sizing policy
    // Strategy specifies target proportion via signal.quantity
    WU_PortfolioParams params = {
        .initial_cash = 100000.0,  // $100k initial cash
        .tx_cost_pct = 0.0,        // No fees for simpler math
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .direction = WU_DIRECTION_LONG,
        .borrow_rate = 0.0,
        .borrow_limit = 0.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 1.0      // Use 100% of strategy-specified allocation
        }
    };
    
    // Create 3-asset portfolio
    const char* symbols[] = {"SPY", "QQQ", "TLT", NULL};
    
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    
    // Test 1: Strategy specifies 50% SPY, 30% QQQ, 20% TLT
    // Portfolio value = $100k
    WU_Signal signals[3];
    
    signals[0].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    signals[0].side = WU_SIDE_BUY;
    signals[0].price = 100.0;
    signals[0].quantity = 0.5;  // 50% target allocation
    
    signals[1].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    signals[1].side = WU_SIDE_BUY;
    signals[1].price = 50.0;
    signals[1].quantity = 0.3;  // 30% target allocation
    
    signals[2].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    signals[2].side = WU_SIDE_BUY;
    signals[2].price = 100.0;
    signals[2].quantity = 0.2;  // 20% target allocation
    
    wu_portfolio_update((WU_Portfolio)portfolio, signals);
    
    // Verify allocations
    // SPY: 50% of $100k = $50k at $100/share → 500 shares
    // QQQ: 30% of $100k = $30k at $50/share → 600 shares
    // TLT: 20% of $100k = $20k at $100/share → 200 shares
    
    double spy_qty = wu_basic_portfolio_asset_quantity(portfolio, 0);
    double qqq_qty = wu_basic_portfolio_asset_quantity(portfolio, 1);
    double tlt_qty = wu_basic_portfolio_asset_quantity(portfolio, 2);
    
    CU_ASSERT_DOUBLE_EQUAL(spy_qty, 500.0, 1.0);  // ~500 shares
    CU_ASSERT_DOUBLE_EQUAL(qqq_qty, 600.0, 1.0);  // ~600 shares
    CU_ASSERT_DOUBLE_EQUAL(tlt_qty, 200.0, 1.0);  // ~200 shares
    
    // Verify values
    double spy_value = wu_basic_portfolio_asset_value(portfolio, 0);
    double qqq_value = wu_basic_portfolio_asset_value(portfolio, 1);
    double tlt_value = wu_basic_portfolio_asset_value(portfolio, 2);
    
    CU_ASSERT_DOUBLE_EQUAL(spy_value, 50000.0, 100.0);  // ~$50k
    CU_ASSERT_DOUBLE_EQUAL(qqq_value, 30000.0, 100.0);  // ~$30k
    CU_ASSERT_DOUBLE_EQUAL(tlt_value, 20000.0, 100.0);  // ~$20k
    
    // Cash should be nearly depleted (100% allocation)
    CU_ASSERT_TRUE(portfolio->cash < 1000.0);
    
    // Test 2: Strategy changes allocation - now 70% SPY, 20% QQQ, 10% TLT
    // Sell all and reallocate
    signals[0].timestamp = (WU_TimeStamp){.mark = 2000, .units = WU_TIME_UNIT_SECONDS};
    signals[0].side = WU_SIDE_SELL;
    signals[0].price = 105.0;  // SPY up 5%
    signals[0].quantity = 0.0;
    
    signals[1].timestamp = (WU_TimeStamp){.mark = 2000, .units = WU_TIME_UNIT_SECONDS};
    signals[1].side = WU_SIDE_SELL;
    signals[1].price = 52.0;   // QQQ up 4%
    signals[1].quantity = 0.0;
    
    signals[2].timestamp = (WU_TimeStamp){.mark = 2000, .units = WU_TIME_UNIT_SECONDS};
    signals[2].side = WU_SIDE_SELL;
    signals[2].price = 98.0;   // TLT down 2%
    signals[2].quantity = 0.0;
    
    wu_portfolio_update((WU_Portfolio)portfolio, signals);
    
    // All positions should be closed
    CU_ASSERT_DOUBLE_EQUAL(wu_basic_portfolio_asset_quantity(portfolio, 0), 0.0, 0.01);
    CU_ASSERT_DOUBLE_EQUAL(wu_basic_portfolio_asset_quantity(portfolio, 1), 0.0, 0.01);
    CU_ASSERT_DOUBLE_EQUAL(wu_basic_portfolio_asset_quantity(portfolio, 2), 0.0, 0.01);
    
    // Portfolio value should be around $103k (gains from price changes)
    double total_value = wu_portfolio_value((WU_Portfolio)portfolio);
    CU_ASSERT_TRUE(total_value > 102000.0 && total_value < 104000.0);
    
    // Now buy with new allocation: 70% SPY, 20% QQQ, 10% TLT
    signals[0].timestamp = (WU_TimeStamp){.mark = 3000, .units = WU_TIME_UNIT_SECONDS};
    signals[0].side = WU_SIDE_BUY;
    signals[0].price = 105.0;
    signals[0].quantity = 0.7;  // 70% target allocation
    
    signals[1].timestamp = (WU_TimeStamp){.mark = 3000, .units = WU_TIME_UNIT_SECONDS};
    signals[1].side = WU_SIDE_BUY;
    signals[1].price = 52.0;
    signals[1].quantity = 0.2;  // 20% target allocation
    
    signals[2].timestamp = (WU_TimeStamp){.mark = 3000, .units = WU_TIME_UNIT_SECONDS};
    signals[2].side = WU_SIDE_BUY;
    signals[2].price = 98.0;
    signals[2].quantity = 0.1;  // 10% target allocation
    
    wu_portfolio_update((WU_Portfolio)portfolio, signals);
    
    // Verify new allocations proportional to portfolio value
    spy_value = wu_basic_portfolio_asset_value(portfolio, 0);
    qqq_value = wu_basic_portfolio_asset_value(portfolio, 1);
    tlt_value = wu_basic_portfolio_asset_value(portfolio, 2);
    
    total_value = wu_portfolio_value((WU_Portfolio)portfolio);
    
    // Check proportions are approximately correct (allowing for rounding)
    double spy_pct = spy_value / total_value;
    double qqq_pct = qqq_value / total_value;
    double tlt_pct = tlt_value / total_value;
    
    CU_ASSERT_DOUBLE_EQUAL(spy_pct, 0.7, 0.01);   // ~70%
    CU_ASSERT_DOUBLE_EQUAL(qqq_pct, 0.2, 0.01);   // ~20%
    CU_ASSERT_DOUBLE_EQUAL(tlt_pct, 0.1, 0.01);   // ~10%
    
    // Test 3: size_value multiplier (use only 80% of strategy allocation)
    wu_portfolio_delete((WU_Portfolio)portfolio);
    
    params.position_sizing.size_value = 0.8;  // Only 80% of target
    portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    
    // Strategy wants 50% allocation, but we'll only use 80% of that = 40%
    signals[0].timestamp = (WU_TimeStamp){.mark = 4000, .units = WU_TIME_UNIT_SECONDS};
    signals[0].side = WU_SIDE_BUY;
    signals[0].price = 100.0;
    signals[0].quantity = 0.5;  // 50% target, but 80% multiplier = 40% actual
    
    signals[1].timestamp = (WU_TimeStamp){.mark = 4000, .units = WU_TIME_UNIT_SECONDS};
    signals[1].side = WU_SIDE_HOLD;
    signals[1].price = 50.0;
    signals[1].quantity = 0.0;
    
    signals[2].timestamp = (WU_TimeStamp){.mark = 4000, .units = WU_TIME_UNIT_SECONDS};
    signals[2].side = WU_SIDE_HOLD;
    signals[2].price = 100.0;
    signals[2].quantity = 0.0;
    
    wu_portfolio_update((WU_Portfolio)portfolio, signals);
    
    // SPY should have 40% of $100k = $40k at $100/share → 400 shares
    spy_qty = wu_basic_portfolio_asset_quantity(portfolio, 0);
    CU_ASSERT_DOUBLE_EQUAL(spy_qty, 400.0, 1.0);
    
    spy_value = wu_basic_portfolio_asset_value(portfolio, 0);
    CU_ASSERT_DOUBLE_EQUAL(spy_value, 40000.0, 100.0);
    
    wu_portfolio_delete((WU_Portfolio)portfolio);
}

void test_basic_portfolio_invalid_asset_index(void) {
    WU_PortfolioParams params = {
        .initial_cash = 100000.0,
        .tx_cost_pct = 0.0,
        .stop_loss_pct = 0.0,
        .take_profit_pct = 0.0,
        .slippage_pct = 0.0,
        .direction = WU_DIRECTION_BOTH,
        .borrow_rate = 0.05,
        .borrow_limit = 100000.0,
        .position_sizing = {
            .size_type = WU_POSITION_SIZE_PCT,
            .size_value = 1.0
        }
    };
    
    const char* symbols[] = {"SPY", "QQQ", NULL};
    WU_BasicPortfolio portfolio = wu_basic_portfolio_new(params, (const char**)symbols);
    
    // Create signal array with only 1 signal for portfolio with 2 assets
    // This tests that the portfolio handles partial signal arrays gracefully
    WU_Signal signals[1];
    signals[0].timestamp = (WU_TimeStamp){.mark = 1000, .units = WU_TIME_UNIT_SECONDS};
    signals[0].side = WU_SIDE_BUY;
    signals[0].price = 100.0;
    signals[0].quantity = 1.0;
    
    wu_portfolio_update((WU_Portfolio)portfolio, signals);
    
    // Only first asset should be affected, cash should be reduced
    CU_ASSERT_TRUE(portfolio->cash < 100000.0);
    
    wu_portfolio_delete((WU_Portfolio)portfolio);
}

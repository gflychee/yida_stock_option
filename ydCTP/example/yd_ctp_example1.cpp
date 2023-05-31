#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <chrono>
#include "ThostFtdcMdApi.h"
#include "ThostFtdcTraderApi.h"

class MarketDataListener : public CThostFtdcMdSpi
{
public:
	bool has_login = false;

	MarketDataListener(CThostFtdcMdApi* mdapi, const char *theuser, const char *thepassword)
	: api(mdapi), user(theuser), password(thepassword)
	{}

	void OnFrontConnected(void) override
	{
		CThostFtdcReqUserLoginField f{};
		strcpy(f.BrokerID, "BrokerA");
		strcpy(f.UserID, user);
		strcpy(f.Password, password);
		api->ReqUserLogin(&f, 0);
	}

	void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override
	{
		printf("MD OnRspUserLogin() %d\n", pRspInfo->ErrorID);
		if (pRspInfo->ErrorID == 0)
		{
			has_login = true;
		}
	}

	void OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override
	{
		printf("OnRspSubMarketData() %s %d\n", pSpecificInstrument->InstrumentID, pRspInfo->ErrorID);
	}

	void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override
	{
		printf("OnRspUnSubMarketData() %s %d\n", pSpecificInstrument->InstrumentID, pRspInfo->ErrorID);
	}

	void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* md) override
	{
		printf("OnRtnDepthMarketData() %s %s %g\n", md->InstrumentID, md->UpdateTime, md->LastPrice);
	}
	
private:
	CThostFtdcMdApi* const api;
	const char *user;
	const char *password;
};

class TradeListener : public CThostFtdcTraderSpi
{
public:
	TradeListener(CThostFtdcTraderApi* tradeapi, const char *theuser, const char *thepassword)
	: api(tradeapi), user(theuser), password(thepassword)
	{}

	virtual void OnFrontConnected(void)
	{
		CThostFtdcReqAuthenticateField auth_req{};
		strcpy(auth_req.BrokerID, "BrokerA");
		strcpy(auth_req.UserID, user);
		strcpy(auth_req.UserProductInfo, "ProductA");
		strcpy(auth_req.AuthCode, ""); // Use AuthCode/AppID in "YDClient.ini"
		strcpy(auth_req.AppID, "");
		if (api->ReqAuthenticate(&auth_req, 666) != 0)
		{
			printf("CTP authentication request failed\n");
		}
	}

	void OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo->ErrorID != 0)
		{
			printf("CTP Trader %s Authentication Failed:%s\n", pRspAuthenticateField->UserID, pRspInfo->ErrorMsg);
		}
		else
		{
			printf("CTP Trader %s Authentication Succeeded\n", pRspAuthenticateField->UserID);

			CThostFtdcReqUserLoginField f{};
			strcpy(f.BrokerID, "BrokerA");
			strcpy(f.UserID, user);
			strcpy(f.Password, password);
			api->ReqUserLogin(&f, 0);
		}
	};

	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("Trade OnRspUserLogin() %d\n", pRspInfo->ErrorID);
	}

	virtual void OnRtnOrder(CThostFtdcOrderField* order)
	{
		printf("Order status notify: ref %s, sysid %s, status %c\n", order->OrderRef, order->OrderSysID, order->OrderStatus);
	}

	virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) 
	{
		printf("Error: Order ref %s, %d/%s \n", pInputOrder->OrderRef, pRspInfo->ErrorID, pRspInfo->ErrorMsg);		
	}

	void OnRtnTrade(CThostFtdcTradeField* trade)
	{
		printf("Trade notify: order ref %s, order sysid %s, tradeid %s, volume %d, price %g\n", trade->OrderRef, trade->OrderSysID, trade->TradeID, trade->Volume, trade->Price);
	}

private:
	CThostFtdcTraderApi* api;
	const char *user;
	const char *password;
};

int main(int argc, const char *argv[])
 {
	 if (argc!=6)
	 {
		 printf("Usage: %s <yd server tcp trade url> <user> <password> <instrument id> <price>\n", argv[0]);
		 return 1;
	 }
	 
	auto trade_api = CThostFtdcTraderApi::CreateFtdcTraderApi();
	TradeListener trade_listener(trade_api, argv[2], argv[3]);
	trade_api->RegisterSpi(&trade_listener);
	trade_api->RegisterFront((char*)argv[1]);
	trade_api->Init();
	printf("Starting trade API\n");

	auto md_api = CThostFtdcMdApi::CreateFtdcMdApi();
	MarketDataListener md_listener(md_api, argv[2], argv[3]);
	md_api->RegisterSpi(&md_listener);
	md_api->RegisterFront((char*)argv[1]);
	md_api->Init();
	printf("Starting marketdata API\n");

	while (!md_listener.has_login)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	
	// Subscrible/unsunscribe market data
	printf("Subscribe/unsubscribe marketdata\n");
	char* md[] = { (char *)argv[4] };
	auto result = md_api->SubscribeMarketData(md, 1);
	printf("subscribe md result %d ", result);
	std::this_thread::sleep_for(std::chrono::seconds(5));
	result = md_api->UnSubscribeMarketData(md, 1);
	printf("Unsubscribe md result %d ", result); 
	
	// Insert an order
	printf("Put an order\n");
	CThostFtdcInputOrderField order{};
	strcpy(order.InstrumentID, argv[4]);
	sprintf(order.OrderRef, "%012d", 666);
	order.Direction = THOST_FTDC_D_Buy;
	order.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
	order.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
	order.LimitPrice = atof(argv[5]);
	order.VolumeTotalOriginal = 1;
	order.TimeCondition = THOST_FTDC_TC_GFD;
	order.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	order.VolumeCondition = THOST_FTDC_VC_AV;
	result = trade_api->ReqOrderInsert(&order, 0);
	printf("Insert Order result %d\n", result);
	std::this_thread::sleep_for(std::chrono::seconds(5));	
 }

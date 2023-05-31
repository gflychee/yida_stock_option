cp ../thosttraderapi_se.so ../thostmduserapi_se.so ../yd.so .
g++ -I.. -o yd_ctp_example1 yd_ctp_example1.cpp ./thosttraderapi_se.so ./thostmduserapi_se.so ./yd.so

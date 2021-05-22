#include <fstream>
#include<iostream>
#include<vector>
#include<string>
#include<sstream>
#include<pcap.h>
#include<winsock2.h>

const char X[] = { "0123456789ABCDEF" };


struct ether_header {
	unsigned char dest_addr[6];		// 目的地址(Destination address)
	unsigned char sour_addr[6];		// 源地址(Source address)
	char type[2];					// 类型

	/*格式化目的地址*/
	std::string get_dest() {
		std::string rt;
		for (int i = 0; i < 6; i++) {
			int now = dest_addr[i];
			rt += X[now / 16];
			rt += X[now % 16];
			if (i != 5) {
				rt += '-';
			}
		}
		return rt;
	}

	/*格式化源地址*/
	std::string get_sour() {
		std::string rt;
		for (int i = 0; i < 6; i++) {
			int now = sour_addr[i];
			rt += X[now / 16];
			rt += X[now % 16];
			if (i != 5) {
				rt += '-';
			}
		}
		return rt;
	}
};

/* 4字节的IP地址 */
typedef struct ip_address {
	u_char byte[4];
}ip_address;

/* IPv4 首部 */
typedef struct ip_header {
	u_char  ver_ihl;        // 版本 (4 bits) + 首部长度 (4 bits)
	u_char  tos;            // 服务类型(Type of service) 
	u_short tlen;           // 总长(Total length) 
	u_short identification; // 标识(Identification)
	u_short flags_fo;       // 标志位(Flags) (3 bits) + 段偏移量(Fragment offset) (13 bits)
	u_char  ttl;            // 存活时间(Time to live)
	u_char  proto;          // 协议(Protocol)
	u_short crc;            // 首部校验和(Header checksum)
	ip_address  sour_addr;  // 源地址(Source address)
	ip_address  dest_addr;  // 目的地址(Destination address)
	u_int   op_pad;         // 选项与填充(Option + Padding)
}ip_header;



/* 回调函数原型 */
void packet_handler(u_char* param, const struct pcap_pkthdr* header, const u_char* pkt_data);

std::ofstream file("out.csv");


int main()
{
	pcap_if_t* alldevs;
	pcap_if_t* d;
	int inum;
	int i = 0;
	pcap_t* adhandle;
	char errbuf[PCAP_ERRBUF_SIZE];
	u_int netmask;
	//char packet_filter[] = "ip and udp";
	char packet_filter[] = "ip";
	struct bpf_program fcode;

	/* 获得设备列表 */
	if (pcap_findalldevs_ex((char*)PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1)
	{
		fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
		exit(1);
	}

	/* 打印列表 */
	for (d = alldevs; d; d = d->next)
	{
		printf("%d. %s", ++i, d->name);
		if (d->description)
			printf(" (%s)\n", d->description);
		else
			printf(" (No description available)\n");
	}

	if (i == 0)
	{
		printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
		return -1;
	}

	printf("Enter the interface number (1-%d):", i);
	scanf("%d", &inum);

	if (inum < 1 || inum > i)
	{
		printf("\nInterface number out of range.\n");
		/* 释放设备列表 */
		pcap_freealldevs(alldevs);
		return -1;
	}

	/* 跳转到已选设备 */
	for (d = alldevs, i = 0; i < inum - 1; d = d->next, i++);

	/* 打开适配器 */
	if ((adhandle = pcap_open(d->name,  // 设备名
		65536,     // 要捕捉的数据包的部分 
				   // 65535保证能捕获到不同数据链路层上的每个数据包的全部内容
		PCAP_OPENFLAG_PROMISCUOUS,         // 混杂模式
		1000,      // 读取超时时间
		NULL,      // 远程机器验证
		errbuf     // 错误缓冲池
	)) == NULL)
	{
		fprintf(stderr, "\nUnable to open the adapter. %s is not supported by WinPcap\n");
		/* 释放设备列表 */
		pcap_freealldevs(alldevs);
		return -1;
	}

	/* 检查数据链路层 */
	if (pcap_datalink(adhandle) != DLT_EN10MB)
	{
		fprintf(stderr, "\nThis program works only on Ethernet networks.\n");
		/* 释放设备列表 */
		pcap_freealldevs(alldevs);
		return -1;
	}

	if (d->addresses != NULL)
		/* 获得接口第一个地址的掩码 */
		netmask = ((struct sockaddr_in*)(d->addresses->netmask))->sin_addr.S_un.S_addr;
	else
		/* 如果接口没有地址，那么我们假设一个C类的掩码 */
		netmask = 0xffffff;


	//编译过滤器
	if (pcap_compile(adhandle, &fcode, packet_filter, 1, netmask) < 0)
	{
		fprintf(stderr, "\nUnable to compile the packet filter. Check the syntax.\n");
		/* 释放设备列表 */
		pcap_freealldevs(alldevs);
		return -1;
	}

	//设置过滤器
	if (pcap_setfilter(adhandle, &fcode) < 0)
	{
		fprintf(stderr, "\nError setting the filter.\n");
		/* 释放设备列表 */
		pcap_freealldevs(alldevs);
		return -1;
	}

	printf("\nlistening on %s...\n", d->name);

	/* 释放设备列表 */
	pcap_freealldevs(alldevs);

	/* 开始捕捉 */
	pcap_loop(adhandle, 0, packet_handler, NULL);

	return 0;
}

/* 回调函数，当收到每一个数据包时会被libpcap所调用 */
void packet_handler(u_char* param, const struct pcap_pkthdr* header, const u_char* pkt_data)
{
	struct tm* ltime;
	char timestr[50];
	ip_header* ih;
	ether_header* eh;
	time_t local_tv_sec;

	/* 将时间戳转换成可识别的格式 */
	time(&local_tv_sec);
	ltime = localtime(&local_tv_sec);
	strftime(timestr, sizeof timestr, "%Y-%m-%d %H:%M:%S", ltime);

	/*获得以太网头部*/
	eh = (ether_header*)pkt_data;

	/* 获得IP数据包头部的位置 */
	ih = (ip_header*)(pkt_data + 14); //以太网头部长度  14字节为mac头长度

	/*输出至日志文件*/
	file << timestr << ",";
	file << eh->get_sour() << ",";
	for (int i = 0; i < 4; i++) {
		file << (int)ih->sour_addr.byte[i];
		if (i != 3) {
			file << ".";
		}
	}
	file << ",";
	file << eh->get_dest() << ",";
	for (int i = 0; i < 4; i++) {
		file << (int)ih->dest_addr.byte[i];
		if (i != 3) {
			file << ".";
		}
	}
	file << "\n";
}

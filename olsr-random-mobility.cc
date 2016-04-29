#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-helper.h"

using namespace ns3;

int 
main (int argc, char *argv[])
{
	int sides = 5;
	bool mobilityStatic = true;
	Ssid ssid;
  	Address serverAddress;

	CommandLine cmd;
	cmd.AddValue ("sides", "Number of sides", sides);
	cmd.AddValue ("mobilityStatic", "Mobility static", mobilityStatic);

	cmd.Parse (argc, argv);

	if(sides < 2) {
		sides = 2;
	}

	// Stack WI-FI
	WifiHelper wifi = WifiHelper::Default ();
	wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
	YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
	wifiPhy.SetChannel(wifiChannel.Create());

	ssid = Ssid("wifi-default");
	wifi.SetRemoteStationManager("ns3::ArfWifiManager");
	wifiMac.SetType ("ns3::AdhocWifiMac", "Ssid", SsidValue(ssid));	

	// Stack Internet
	InternetStackHelper internet = InternetStackHelper();
	Ipv4ListRoutingHelper list_routing = Ipv4ListRoutingHelper();
	OlsrHelper olsr_routing = OlsrHelper();
	Ipv4StaticRoutingHelper static_routing = Ipv4StaticRoutingHelper();
	list_routing.Add(static_routing, 0);
	list_routing.Add(olsr_routing, 10);
	internet.SetRoutingHelper(list_routing);

	Ipv4AddressHelper ipv4Addresses = Ipv4AddressHelper();
	ipv4Addresses.SetBase(Ipv4Address("10.0.0.0"), Ipv4Mask("255.255.255.0"));

	// Creamos el contenedor de nodos
	NodeContainer c = NodeContainer();
	c.Create (sides*sides);

	MobilityHelper mobility;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();
	int punt = 0;

	int distance = (200 / sides);
	positionAlloc ->Add(Vector( -100, - 100, 0));

	internet.Install(c);
	NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);
	Ipv4InterfaceContainer Ipv4InterfaceContainer = ipv4Addresses.Assign (devices);
	wifiPhy.EnablePcapAll ("olsr-random-mobility", false);

	uint16_t port = 9;  // well-known echo port number
	UdpEchoServerHelper server (port);
	ApplicationContainer apps = server.Install (c.Get (0));
	apps.Start (Seconds (1.0));
	apps.Stop (Seconds (10.0));
	serverAddress = Address(Ipv4InterfaceContainer.GetAddress (0));
	Time interPacketInterval = Seconds (1.);
	uint32_t packetSize = 1024;
	uint32_t maxPacketCount = 1;
	UdpEchoClientHelper client (serverAddress, port);
	client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
	client.SetAttribute ("Interval", TimeValue (interPacketInterval));
	client.SetAttribute ("PacketSize", UintegerValue (packetSize));

	for( int i = 0; i < sides; i++ )	
	{
		for ( int j = 0; j < sides; j++)
		{
			if(punt != 0)
			{
				apps = client.Install (c.Get (punt));
				apps.Start (Seconds (2.0));
				apps.Stop (Seconds (10.0));
				positionAlloc ->Add(Vector( i*distance - 100, j*distance - 100, 0));
			}	
			punt++;
		}
	}

	mobility.SetPositionAllocator(positionAlloc);
	if(mobilityStatic){
		mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	}
	else{
		mobility.SetMobilityModel("ns3::RandomDirection2dMobilityModel");
	}

	mobility.Install(c);

	Simulator::Stop (Seconds (20.0));
	Simulator::Run ();
	Simulator::Destroy ();

	return 0;
}

#!/usr/bin/env python3
from scapy.all import *
import ipaddress
import time
import signal
import sys

xv6_ip = input("Enter the IP of xv6: ").strip()

try:
    ipaddress.IPv4Address(xv6_ip)
except ipaddress.AddressValueError:
    print("Invalid IPv4 address")
    exit(1)

iface = "tap0"

def get_mac(ip):
    arp_req = Ether(dst="ff:ff:ff:ff:ff:ff") / ARP(pdst=ip)
    resp = srp1(arp_req, iface=iface, timeout=2, verbose=0)
    if resp and ARP in resp:
        return resp[ARP].hwsrc
    else:
        return None


xv6_mac = get_mac(xv6_ip)
if not xv6_mac:
    print(f"Could not resolve MAC for {xv6_ip} via ARP")
    exit(1)
print(f"MAC of {xv6_ip} is {xv6_mac}")

sent = 0
received = 0
seq = 1
icmp_id = 1234
payload = b"PINGPONG"

def signal_handler(sig, frame):
    print("\n--- Ping statistics ---")
    print(
        f"{sent} packets transmitted, {received} packets received, "
        f"{100 * (sent - received) / sent:.1f}% packet loss"
    )
    sys.exit(0)


signal.signal(signal.SIGINT, signal_handler)

while True:
    pkt = (
        Ether(dst=xv6_mac)
        / IP(dst=xv6_ip)
        / ICMP(id=icmp_id, seq=seq)
        / Raw(load=payload)
    )
    sent += 1
    resp = srp1(pkt, iface=iface, timeout=2, verbose=0)

    if resp and ICMP in resp:
        data_bytes = resp[Raw].load 
        data_str = data_bytes.decode(errors='replace')
        received += 1
        print(
            f"[{seq}] Reply from {resp[IP].src}: type={resp[ICMP].type}, payload='{data_str}'"
        )
    else:
        print(f"[{seq}] No reply")

    seq += 1
    time.sleep(1)

#!/usr/bin/env atf-sh
#-
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2022 En-Wei Wu
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# $FreeBSD$
#

atf_test_case "mesh_ping_test" "cleanup"
mesh_ping_test_head() {
	atf_set descr 'Test mesh mode iface created by wtap and do icmp test'
	atf_set require.user root
	atf_set require.progs jail
}

mesh_ping_test_body() {
	if ! kldstat -q -m wtap; then
		atf_skip "This test requires wtap.ko"
	fi

	# XXX we shouldn't use files in tools. In the future, we will
	# have a "wtapctl" in either tests directory or /bin (/sbin)
	tools_dir_mk=/usr/src/tools/tools/wtap
	tools_dir=$(cd $tools_dir_mk && make -V .OBJDIR)

	${tools_dir}/wtap/wtap c 0
	${tools_dir}/wtap/wtap c 1

	${tools_dir}/vis_map/vis_map o
	${tools_dir}/vis_map/vis_map a 0 1
	${tools_dir}/vis_map/vis_map a 1 0

	mesh_a_nm=$(ifconfig wlan create wlandev wtap0 wlanmode mesh meshid wtap_test)
	mesh_b_nm=$(ifconfig wlan create wlandev wtap1 wlanmode mesh meshid wtap_test)

	# record name for cleanup
	echo ${mesh_a_nm} > mesh_a_nm.txt
	echo ${mesh_b_nm} > mesh_b_nm.txt

	jname="mesh_ping_test"
	jail -c name=${jname} persist vnet vnet.interface=${mesh_a_nm} allow.raw_sockets

	mesh_a_ip="10.0.0.1"
	mesh_a_cidr="${mesh_a_ip}/24"
	mesh_b_ip="10.0.0.2"
	mesh_b_cidr="${mesh_b_ip}/24"

	jexec ${jname} ifconfig ${mesh_a_nm} inet ${mesh_a_cidr} up
	ifconfig ${mesh_b_nm} inet ${mesh_b_cidr} up

	# XXX for unkown reason, mesh mode needs some time to prepare
	sleep 10

	# wait for two mesh interfaces get ready
	for i in $(seq 1 1 5); do
		mesh_a_bssid=$(jexec ${jname} ifconfig ${mesh_a_nm} | grep bssid | awk '{print $9}')
		mesh_b_bssid=$(ifconfig ${mesh_b_nm} | grep bssid | awk '{print $9}')
		
		if [ -n "${mesh_a_bssid}" ] && [ -n "${mesh_b_bssid}" ]; then
			ping -S ${mesh_b_ip} -c 2 ${mesh_a_ip}
			
			if [ $? -eq 0 ]; then
				success=1
			fi
			break
		fi

		sleep 1
	done

	atf_check_equal 1 $success
}

mesh_ping_test_cleanup() {
	if ! kldstat -q -m wtap; then
		atf_skip "This test requires wtap.ko"
	fi

	tools_dir_mk=/usr/src/tools/tools/wtap
	tools_dir=$(cd $tools_dir_mk && make -V .OBJDIR)

	mesh_a_nm=$(cat mesh_a_nm.txt)
	mesh_b_nm=$(cat mesh_b_nm.txt)

	rm -f mesh_a_nm.txt mesh_b_nm.txt
	
	jname="mesh_ping_test"
	jexec ${jname} ifconfig ${mesh_a_nm} destroy
	ifconfig ${mesh_b_nm} destroy

	jail -r ${jname}

	${tools_dir}/wtap/wtap d 0
	${tools_dir}/wtap/wtap d 1
}

atf_test_case "adhoc_ping_test" "cleanup"
adhoc_ping_test_head() {
	atf_set descr 'Test adhoc mode iface created by wtap and do icmp test'
	atf_set require.user root
	atf_set require.progs jail
}

adhoc_ping_test_body() {
	if ! kldstat -q -m wtap; then
		atf_skip "This test requires wtap.ko"
	fi
	
	# XXX we shouldn't use files in tools. In the future, we will
	# have a "wtapctl" in either tests directory or /bin (/sbin)
	tools_dir_mk=/usr/src/tools/tools/wtap
	tools_dir=$(cd $tools_dir_mk && make -V .OBJDIR)

	${tools_dir}/wtap/wtap c 0
	${tools_dir}/wtap/wtap c 1

	${tools_dir}/vis_map/vis_map o
	${tools_dir}/vis_map/vis_map a 0 1
	${tools_dir}/vis_map/vis_map a 1 0

	adhoc_a_nm=$(ifconfig wlan create wlandev wtap0 wlanmode adhoc ssid wtap_test)
	adhoc_b_nm=$(ifconfig wlan create wlandev wtap1 wlanmode adhoc ssid wtap_test)

	# record name for cleanup
	echo ${adhoc_a_nm} > adhoc_a_nm.txt
	echo ${adhoc_b_nm} > adhoc_b_nm.txt

	jname="adhoc_ping_test"
	jail -c name=${jname} persist vnet vnet.interface=${adhoc_a_nm} allow.raw_sockets

	adhoc_a_ip="10.0.0.1"
	adhoc_a_cidr="${adhoc_a_ip}/24"
	adhoc_b_ip="10.0.0.2"
	adhoc_b_cidr="${adhoc_b_ip}/24"

	jexec ${jname} ifconfig ${adhoc_a_nm} inet ${adhoc_a_cidr} up
	ifconfig ${adhoc_b_nm} inet ${adhoc_b_cidr} up

	# wait for ibss merge, and do ping test on success
	for i in $(seq 1 1 5); do
		adhoc_a_bssid=$(jexec ${jname} ifconfig ${adhoc_a_nm} | grep bssid | awk '{print $9}')
		adhoc_b_bssid=$(ifconfig ${adhoc_b_nm} | grep bssid | awk '{print $9}')
		
		if [ -n "${adhoc_a_bssid}" ] && [ -n "${adhoc_b_bssid}" ] \
			&& [ "${adhoc_a_bssid}" == "${adhoc_b_bssid}" ]; then
			ping -S ${adhoc_b_ip} -c 2 ${adhoc_a_ip}
			
			if [ $? -eq 0 ]; then
				success=1
			fi
			break
		fi

		sleep 1
	done

	atf_check_equal 1 $success
}

adhoc_ping_test_cleanup() {
	tools_dir_mk=/usr/src/tools/tools/wtap
	tools_dir=$(cd $tools_dir_mk && make -V .OBJDIR)

	adhoc_a_nm=$(cat adhoc_a_nm.txt)
	adhoc_b_nm=$(cat adhoc_b_nm.txt)

	rm -f adhoc_a_nm.txt adhoc_b_nm.txt
	
	jname="adhoc_ping_test"
	jexec ${jname} ifconfig ${adhoc_a_nm} destroy
	ifconfig ${adhoc_b_nm} destroy

	jail -r ${jname}

	${tools_dir}/wtap/wtap d 0
	${tools_dir}/wtap/wtap d 1
}

atf_test_case "sta_hostap_ping_test" "cleanup"
sta_hostap_ping_test_head() {
	atf_set descr 'Test sta/hostap mode iface created by wtap and do icmp test'
	atf_set require.user root
	atf_set require.progs jail
}

sta_hostap_ping_test_body() {
	if ! kldstat -q -m wtap; then
		atf_skip "This test requires wtap.ko"
	fi

	# XXX we shouldn't use files in tools. In the future, we will
	# have a "wtapctl" in either tests directory or /bin (/sbin)
	tools_dir_mk=/usr/src/tools/tools/wtap
	tools_dir=$(cd $tools_dir_mk && make -V .OBJDIR)

	${tools_dir}/wtap/wtap c 0
	${tools_dir}/wtap/wtap c 1

	${tools_dir}/vis_map/vis_map o
	${tools_dir}/vis_map/vis_map a 0 1
	${tools_dir}/vis_map/vis_map a 1 0

	hostap_nm=$(ifconfig wlan create wlandev wtap0 wlanmode hostap ssid wtap_test)
	sta_nm=$(ifconfig wlan create wlandev wtap1 wlanmode sta ssid wtap_test)

	# record name for cleanup
	echo ${hostap_nm} > hostap_nm.txt
	echo ${sta_nm} > sta_nm.txt

	jname="sta_hostap_ping_test"
	jail -c name=${jname} persist vnet vnet.interface=${hostap_nm} allow.raw_sockets

	hostap_ip="10.0.0.1"
	hostap_cidr="${hostap_ip}/24"
	sta_ip="10.0.0.2"
	sta_cidr="${sta_ip}/24"


	jexec ${jname} ifconfig ${hostap_nm} inet ${hostap_cidr} up
	ifconfig ${sta_nm} inet ${sta_cidr} up

	# wait for hostap gets ready
	for i in $(seq 1 1 5); do
		hostap_bssid=$(jexec ${jname} ifconfig ${hostap_nm} | grep bssid | awk '{print $9}')

		if [ -n "${hostap_bssid}" ]; then
			break
		fi

		sleep 1
	done

	$(atf_get_srcdir)/sta_assoc ${sta_nm} ${hostap_bssid}

	# wait for sta assoc with hostap, and do ping test on success
	for i in $(seq 1 1 5); do
		sta_bssid=$(ifconfig ${sta_nm} | grep bssid | awk '{print $9}')
		
		if [ -n "${hostap_bssid}" ] && [ -n "${sta_bssid}" ] \
			&& [ "${hostap_bssid}" == "${sta_bssid}" ]; then
			ping -S ${sta_ip} -c 2 ${hostap_ip}

			if [ $? -eq 0 ]; then
				success=1
			fi
			break
		fi

		sleep 1
	done

	atf_check_equal 1 $success
}

sta_hostap_ping_test_cleanup() {
	tools_dir_mk=/usr/src/tools/tools/wtap
	tools_dir=$(cd $tools_dir_mk && make -V .OBJDIR)

	hostap_nm=$(cat hostap_nm.txt)
	sta_nm=$(cat sta_nm.txt)

	rm -f hostap_nm.txt sta_nm.txt
	
	jname="sta_hostap_ping_test"
	jexec ${jname} ifconfig ${hostap_nm} destroy
	ifconfig ${sta_nm} destroy

	jail -r ${jname}

	${tools_dir}/wtap/wtap d 0
	${tools_dir}/wtap/wtap d 1
}

atf_test_case "monitor_tcpdump_check" "cleanup"
monitor_tcpdump_check_head() {
	atf_set descr 'Test monitor mode iface created by wtap and do ping/tcpdump test'
	atf_set require.user root
	atf_set require.progs jail tcpdump
}

monitor_tcpdump_check_body() {
	if ! kldstat -q -m wtap; then
		atf_skip "This test requires wtap.ko"
	fi

	# XXX we shouldn't use files in tools. In the future, we will
	# have a "wtapctl" in either tests directory or /bin (/sbin)
	tools_dir_mk=/usr/src/tools/tools/wtap
	tools_dir=$(cd $tools_dir_mk && make -V .OBJDIR)

	${tools_dir}/wtap/wtap c 0
	${tools_dir}/wtap/wtap c 1
	${tools_dir}/wtap/wtap c 2

	${tools_dir}/vis_map/vis_map o
	${tools_dir}/vis_map/vis_map a 0 1
	${tools_dir}/vis_map/vis_map a 0 2
	${tools_dir}/vis_map/vis_map a 1 0
	${tools_dir}/vis_map/vis_map a 1 2
	${tools_dir}/vis_map/vis_map a 2 0
	${tools_dir}/vis_map/vis_map a 2 1

	hostap_nm=$(ifconfig wlan create wlandev wtap0 wlanmode hostap ssid wtap_test)
	sta_nm=$(ifconfig wlan create wlandev wtap1 wlanmode sta ssid wtap_test)
	monitor_nm=$(ifconfig wlan create wlandev wtap2 wlanmode monitor)

	# record name for cleanup
	echo ${hostap_nm} > hostap_nm.txt
	echo ${sta_nm} > sta_nm.txt
	echo ${monitor_nm} > monitor_nm.txt

	jname="monitor_tcpdump_check"
	jail -c name=${jname} persist vnet vnet.interface=${hostap_nm} allow.raw_sockets

	hostap_ip="10.0.0.1"
	hostap_cidr="${hostap_ip}/24"
	sta_ip="10.0.0.2"
	sta_cidr="${sta_ip}/24"
	monitor_ip="10.0.0.3"
	monitor_cidr="${monitor_ip}/24"

	jexec ${jname} ifconfig ${hostap_nm} inet ${hostap_cidr} up
	ifconfig ${sta_nm} inet ${sta_cidr} up
	ifconfig ${monitor_nm} inet ${monitor_cidr} up

	# wait for hostap gets ready
	for i in $(seq 1 1 5); do
		hostap_bssid=$(jexec ${jname} ifconfig ${hostap_nm} | grep bssid | awk '{print $9}')

		if [ -n "${hostap_bssid}" ]; then
			break
		fi

		sleep 1
	done

	$(atf_get_srcdir)/sta_assoc ${sta_nm} ${hostap_bssid}

	# wait for sta assoc with hostap
	for i in $(seq 1 1 5); do
		sta_bssid=$(ifconfig ${sta_nm} | grep bssid | awk '{print $9}')
		
		if [ -n "${hostap_bssid}" ] && [ -n "${sta_bssid}" ] \
			&& [ "${hostap_bssid}" == "${sta_bssid}" ]; then
			assoc=1
			break
		fi

		sleep 1
	done

	# do ping test, packets flow between sta and hostap should be capture
	# by monitor interface, using tcpdump(1)
	if [ "${assoc}" == "1" ]; then
		tcpdump -y IEEE802_11_RADIO -i ${monitor_nm} -w tcpdump.out \
			"icmp and (src ${hostap_ip} or ${sta_ip}) and (dst ${hostap_ip} or ${sta_ip})" &
		ping -S ${sta_ip} -c 5 ${hostap_ip}

		pkill tcpdump

		# may need to wait a while for tcpdump(1) writing captured packets into tcpdump.out
		for i in $(seq 1 1 5); do
			if [ -s "tcpdump.out" ]; then
				success=1
				break
			fi

			sleep 1
		done
	fi

	atf_check_equal 1 $success
}

monitor_tcpdump_check_cleanup() {
	tools_dir_mk=/usr/src/tools/tools/wtap
	tools_dir=$(cd $tools_dir_mk && make -V .OBJDIR)

	rm -f tcpdump.out

	hostap_nm=$(cat hostap_nm.txt)
	sta_nm=$(cat sta_nm.txt)
	monitor_nm=$(cat monitor_nm.txt)

	rm -f hostap_nm.txt sta_nm.txt monitor_nm.txt
	
	jname="monitor_tcpdump_check"
	jexec ${jname} ifconfig ${hostap_nm} destroy
	ifconfig ${sta_nm} destroy
	ifconfig ${monitor_nm} destroy

	jail -r ${jname}

	${tools_dir}/wtap/wtap d 0
	${tools_dir}/wtap/wtap d 1
	${tools_dir}/wtap/wtap d 2
}

atf_init_test_cases() {
	atf_add_test_case "mesh_ping_test"
	atf_add_test_case "adhoc_ping_test"
	atf_add_test_case "sta_hostap_ping_test"
	atf_add_test_case "monitor_tcpdump_check"
}

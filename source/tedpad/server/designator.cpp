#include "../../../include/tedpad/server/designator.h"

bool tedpad::intern_server::Designator::state_clientPending() const
{
	std::lock_guard<std::mutex> lx_state(pmx_state);
	return(pm_state[State_e::ClientPending]);
}

tedpad::intern_server::ImplementationClientInfo tedpad::intern_server::Designator::get_pendingClientInfo(bool const forget)
{
	std::lock_guard<std::mutex> lx_state(pmx_state);
	std::lock_guard<std::mutex> lx_pendingClientInfo(pmx_pendingClientInfo);
	if (client_pending()) {
		auto rtrn = pm_pendingClientInfo.at(0);
		if (forget) {
			pm_pendingClientInfo.erase(pm_pendingClientInfo.begin());
			if (pm_pendingClientInfo.empty())
				pm_state[State_e::ClientPending] = false;
		}
		return(rtrn);
	}
	return(ImplementationClientInfo());
}

tedpad::intern_server::ImplementationClientInfo tedpad::intern_server::Designator::get_pendingClientInfo() const
{
	std::lock_guard<std::mutex> lx_pendingClientInfo(pmx_pendingClientInfo);
	return(client_pending() ? pm_pendingClientInfo.at(0) : ImplementationClientInfo());
}

void tedpad::intern_server::Designator::set_port(uint16_t const port)
{
	std::lock_guard<std::mutex> lx_state(ThreadedObject::pmx_state);
	if (!ThreadedObject::pm_state[ThreadedObject::State_e::ThreadRunning]) {
		std::lock_guard<std::mutex> lx_port(pmx_port);
		pm_port = port;
	}
}

uint16_t tedpad::intern_server::Designator::get_port() const
{
	std::lock_guard<std::mutex> lx_port(pmx_port);
	return(pm_port);
}

tedpad::intern_server::Designator::Designator(UpdateSignal const &updateSignal, uint16_t const port, std::chrono::milliseconds const &updateRate) :
	pm_state(1),
	pm_updateSignal(updateSignal),
	pm_port(port),
	SleepObject(updateRate)
{
}

void tedpad::intern_server::Designator::thread_main()
{
	sockaddr_storage client_address;
	socklen_t client_address_len = sizeof(sockaddr_storage);

	SOCKET client_socket = accept(pm_socket, reinterpret_cast<sockaddr*>(&client_address), &client_address_len);
	if (client_socket != INVALID_SOCKET) {
		if (client_address.ss_family == AF_INET) {
			std::lock_guard<std::mutex> lx_state(pmx_state);
			std::lock_guard<std::mutex> lx_pendingClientInfo(pmx_pendingClientInfo);

			sockaddr_in *client_addr = reinterpret_cast<sockaddr_in*>(&client_address);

			ImplementationClientInfo clientInfo;
			clientInfo.ip = ntohl(client_addr->sin_addr.s_addr);
			clientInfo.port = ntohs(client_addr->sin_port);
			clientInfo.socket = client_socket;
			pm_pendingClientInfo.push_back(clientInfo);

			pm_state[State_e::ClientPending] = true;

			if (pm_updateSignal.filled()) {
				std::lock_guard<std::mutex> lx_updateSignal_lock(*pm_updateSignal.lock);
				pm_updateSignal.eventQueue->push_back(UpdateSignal::Event::Designator_NewClient);
				*pm_updateSignal.request = true;
				pm_updateSignal.signal->notify_all();
			}
		}
	}
	else if ((socket_service::get_lastError() != socket_service::ERROR_EWOULDBLOCK) && (socket_service::get_lastError() != socket_service::ERROR_WAGAIN)) {
		std::cout << "tedpad::intern_server::Designator::thread_main(): accept error" << std::endl;
		exit(1);
	}

	pmx_updateRate.lock();
	auto sleep_time = pm_updateRate;
	pmx_updateRate.unlock();

	std::this_thread::sleep_for(sleep_time);
}

void tedpad::intern_server::Designator::thread_init()
{
	std::lock_guard<std::mutex> plx_port(pmx_port);
	std::stringstream port_ss;
	port_ss << pm_port;

	addrinfo getaddrinfo_in;
	addrinfo *getaddrinfo_out;
	int result;
	
	memset(&getaddrinfo_in, 0, sizeof(addrinfo));

	getaddrinfo_in.ai_family = AF_INET;
	getaddrinfo_in.ai_socktype = SOCK_STREAM;
	getaddrinfo_in.ai_flags = AI_PASSIVE;
	result = getaddrinfo(NULL, port_ss.str().c_str(), &getaddrinfo_in, &getaddrinfo_out);
	if (result != 0) {
		//Probably need to lock cout somehow...
		std::cout << "tedpad::intern_server::Designator::thread_init(): getaddrinfo error: " << gai_strerror(result) << std::endl;;
		//TODO: Something better than "exit" around here. (GlbRtrn?)
		exit(1);
	}

	pm_socket = socket(getaddrinfo_out->ai_family, getaddrinfo_out->ai_socktype, getaddrinfo_out->ai_protocol);
	if (pm_socket == INVALID_SOCKET) {
		std::cout << "tedpad::intern_server::Designator::thread_init(): socket error" << std::endl;
		exit(1);
	}

	result = bind(pm_socket, getaddrinfo_out->ai_addr, static_cast<int>(getaddrinfo_out->ai_addrlen));
	if (result == SOCKET_ERROR) {
		std::cout << "tedpad::intern_server::Designator::thread_init(): bind error" << std::endl;
		exit(1);
	}

	freeaddrinfo(getaddrinfo_out);

	result = listen(pm_socket, SOMAXCONN);
	if (result == SOCKET_ERROR) {
		std::cout << "tedpad::intern_server::Designator::thread_init(): listen error" << std::endl;
		exit(1);
	}
	
	if (socket_service::socket_setBlocking(pm_socket, false)) {
		std::cout << "tedpad::intern_server::Designator::thread_init(): socket_setBlocking error" << std::endl;
		exit(1);
	}
}

void tedpad::intern_server::Designator::thread_close()
{
	std::lock_guard<std::mutex> lx_pendingClientInfo(pmx_pendingClientInfo);
	std::for_each(pm_pendingClientInfo.begin(), pm_pendingClientInfo.end(), [](ImplementationClientInfo const &p0) { closesocket(p0.socket); });
	closesocket(pm_socket);
}

bool tedpad::intern_server::Designator::client_pending() const
{
	return(!pm_pendingClientInfo.empty());
}

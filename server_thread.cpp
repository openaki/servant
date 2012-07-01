/*
void server_thread()
{
	struct sockaddr_in my_addr;
	struct sockaddr_in client_addr;
	int yes = 1;
	int sockfd, new_fd;

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = ini.port;
	inet_aton(gethostbyname(ini.hostname), &(my_addr.sin_addr));
	memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));
	
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my,adddr))) {
		perror("Bind");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	while(1) {
		sin_size = sizeof(client_addr);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
			raise(SIGINT);
			continue;
		}
		
		start_connection(new_fd);
	}

	return ;
}
*/	
		




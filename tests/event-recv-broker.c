#include <broker/broker.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/*----------------------------------------------------------------*/
int
check_contents_poll(broker_message_queue* q, broker_vector *expected)
{
	struct pollfd pfd = {broker_message_queue_fd(q), POLLIN, 0};
	
	poll(&pfd, 1, -1);
	
	if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
		return 1;
	
	broker_deque_of_message *msgs = broker_message_queue_need_pop(q);
	int n = broker_deque_of_message_size(msgs);
	int i;

	for (i = 0; i < n; ++i) {
		broker_vector *m = broker_deque_of_message_at(msgs, i);
		broker_vector_iterator *it = broker_vector_iterator_create(m);
		while (!broker_vector_iterator_at_last(m, it)) {
			broker_data *v = broker_vector_iterator_value(it);
			fprintf(stdout,
				"Got a message: %s!\n",
				broker_string_data(broker_data_to_string(v)));
			broker_data_delete(v);
			broker_vector_iterator_next(m, it);
		}
		broker_vector_iterator_delete(it);
		broker_vector_delete(m);
	}
	
	broker_deque_of_message_delete(msgs);

	return 1;
}
/*----------------------------------------------------------------*/
int
main()
{
	broker_init(0);
	broker_endpoint *node0 = broker_endpoint_create("node0");
	broker_string *topic_a = broker_string_create("bro/event");
	broker_message_queue *pq0a = broker_message_queue_create(topic_a, node0);
	broker_vector *pq0a_expected = broker_vector_create();

	if (!broker_endpoint_listen(node0, 9999, "127.0.0.1", 1)) {
		fprintf(stderr, "%s\n", broker_endpoint_last_error(node0));
		return EXIT_FAILURE;
	}

	while (1)
		check_contents_poll(pq0a, pq0a_expected);

	broker_vector_delete(pq0a_expected);
	/*broker_message_queue_delete(pq0a);*/
	broker_string_delete(topic_a);
	broker_endpoint_delete(node0);
	
	return EXIT_SUCCESS;
}
/*----------------------------------------------------------------*/

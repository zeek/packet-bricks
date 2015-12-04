#include <broker/broker.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
/*----------------------------------------------------------------*/
#define COUNTER_IS_MAIN_MSG			2
#define COUNTER_IS_MSG_TYPE			1
/*----------------------------------------------------------------*/
void
print_record(broker_data *v)
{
	broker_record *inner_r = broker_data_as_record(v);
	broker_record_iterator *inner_it = broker_record_iterator_create(inner_r);
	fprintf(stdout, "Got a record\n");
	while (!broker_record_iterator_at_last(inner_r, inner_it)) {
		broker_data *inner_d = broker_record_iterator_value(inner_it);
		if (inner_d != NULL) {
			broker_data_type bdt = broker_data_which(inner_d);
			switch (bdt) {
			case broker_data_type_bool:
				fprintf(stdout, "Got a bool: %d\n",
					broker_bool_true(broker_data_as_bool(inner_d)));
				break;
			case broker_data_type_count:
				fprintf(stdout, "Got a count: %lu\n",
					*broker_data_as_count(inner_d));
				break;
			case broker_data_type_integer:
				fprintf(stdout, "Got an integer: %ld\n",
					*broker_data_as_integer(inner_d));
				break;
			case broker_data_type_real:
				fprintf(stdout, "Got a real: %f\n",
					*broker_data_as_real(inner_d));
				break;
			case broker_data_type_string:
				fprintf(stdout, "Got a string: %s\n",
					broker_string_data(broker_data_as_string(inner_d)));
				break;
			case broker_data_type_address:
				fprintf(stdout, "Got an address: %s\n",
					broker_string_data(broker_address_to_string(broker_data_as_address(inner_d))));
				break;
			case broker_data_type_subnet:
				fprintf(stdout, "Got a subnet: %s\n",
					broker_string_data(broker_subnet_to_string(broker_data_as_subnet(inner_d))));
				break;
			case broker_data_type_port:
				fprintf(stdout, "Got a port: %u\n",
					broker_port_number(broker_data_as_port(inner_d)));
				break;
			case broker_data_type_time:
				fprintf(stdout, "Got time: %f\n",
					broker_time_point_value(broker_data_as_time(inner_d)));
				break;
			case broker_data_type_duration:
				fprintf(stdout, "Got duration: %f\n",
					broker_time_duration_value(broker_data_as_duration(inner_d)));
				break;
			case broker_data_type_enum_value:
				fprintf(stdout, "Got enum: %s\n",
					broker_enum_value_name(broker_data_as_enum(inner_d)));
				break;
			case broker_data_type_set:
				fprintf(stdout, "Got a set\n");
				break;
			case broker_data_type_table:
				fprintf(stdout, "Got a table\n");
				break;
			case broker_data_type_vector:
				fprintf(stdout, "Got a vector\n");
				break;
			case broker_data_type_record:
				print_record(inner_d);
				break;
			default:
				break;
			}
		}
		broker_data_delete(inner_d);
		broker_record_iterator_next(inner_r, inner_it);
	}
	broker_record_iterator_delete(inner_it);
	broker_record_delete(inner_r);
	fprintf(stdout, "End of record\n");
}
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

	/* check vector contents */
	for (i = 0; i < n; ++i) {
		broker_vector *m = broker_deque_of_message_at(msgs, i);
		broker_vector_iterator *it = broker_vector_iterator_create(m);
		int count = 0;
		while (!broker_vector_iterator_at_last(m, it)) {
			broker_data *v = broker_vector_iterator_value(it);
			if (count == COUNTER_IS_MAIN_MSG) {
				print_record(v);
			} else {
				if (v != NULL /*&& broker_data_which(v) != broker_data_type_record*/) {
					if (broker_data_which(v) == broker_data_type_record)
						print_record(v);
					else
						fprintf(stdout,
							"Got a message: %s!\n",
							broker_string_data(broker_data_to_string(v)));
				}
				broker_data_delete(v);
				broker_vector_iterator_next(m, it);
			}
			count++;
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

	while (true)
		check_contents_poll(pq0a, pq0a_expected);

	broker_vector_delete(pq0a_expected);
	/*broker_message_queue_delete(pq0a);*/
	broker_string_delete(topic_a);
	broker_endpoint_delete(node0);
	
	return EXIT_SUCCESS;
}
/*----------------------------------------------------------------*/

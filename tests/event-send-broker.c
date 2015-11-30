#include <broker/broker.h>
#include <stdio.h>
#include <unistd.h>
#define WITH_STRUCT
#ifdef WITH_STRUCT
typedef struct {
	int a;
	int b;
} mystruct;
#endif
/*----------------------------------------------------------------*/
int
main()
{
	broker_init(0);
	
	broker_endpoint* endpoint = broker_endpoint_create("client");
	
	broker_endpoint_peer_remotely(endpoint, "127.0.0.1", 9999, 1.0);
	
	const broker_outgoing_connection_status_queue* status =
		broker_endpoint_outgoing_connection_status(endpoint);
	
	broker_outgoing_connection_status_queue_need_pop(status);
	
	broker_message* msg = broker_vector_create();
	
	broker_string* event_name  = broker_string_create("my_event");
	broker_vector_insert(msg, broker_data_from_string(event_name), 0);
	
	broker_string* event_arg1  = broker_string_create("Hello C Broker!");
	broker_vector_insert(msg, broker_data_from_string(event_arg1), 1);
	
	broker_vector_insert(msg, broker_data_from_count(42), 2);
	
	
#ifdef WITH_STRUCT
	mystruct f;
	f.a = 10;
	f.b = 20;
	broker_data *bd = broker_data_create();
	/* ... */
	broker_vector_insert(msg, bd, 3);
#endif
	
	broker_string* event_topic = broker_string_create("bro/event");
	broker_endpoint_send(endpoint, event_topic, msg);
	
	broker_outgoing_connection_status_queue_need_pop(status);
}
/*----------------------------------------------------------------*/

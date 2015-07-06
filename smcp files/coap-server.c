/*!	@page smcp-example-2 example-2.c: Responding to a specific request
**
**	This example shows how to respond to a request for a specific resource.
**
**	@include example-2.c
**
**	@sa @ref smcp-instance, @ref smcp-inbound, @ref smcp-outbound
*/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <smcp/smcp.h>


static smcp_status_t
request_handler(void* context) {
	smcp_status_t ret = SMCP_STATUS_OK;
	char* content = (char*)smcp_inbound_get_content_ptr();
	size_t content_length = smcp_inbound_get_content_len();
	char *path = NULL;
	FILE * pFile;

	path = smcp_inbound_get_path(NULL,0); // path must be free()'d after this!
	//require_action_string(path && path[0], (free((void*)path),ret = SMCP_STATUS_INVALID_ARGUMENT));
	if(smcp_inbound_get_code() != COAP_METHOD_POST) {
		return SMCP_STATUS_NOT_IMPLEMENTED;
	}

	// Skip to the URI path option
	while(smcp_inbound_peek_option(NULL, NULL) != COAP_OPTION_URI_PATH)
		if(smcp_inbound_next_option(NULL, NULL) == COAP_OPTION_INVALID)
			break;

	smcp_outbound_begin_response(COAP_RESULT_205_CONTENT);

	smcp_outbound_add_option_uint(
		COAP_OPTION_CONTENT_TYPE,
		COAP_CONTENT_TYPE_TEXT_PLAIN);

	if(content && content_length) {
		// if content exist then print and write on .txt file with "a" being append feature.
		pFile = fopen ("data-collect.txt","a");
		printf("%s", content);
		if (pFile!=NULL){
    		fputs (content,pFile);
    		fclose (pFile);
      	}
		
		/* Only print a newline if the content doesn't already print one. */
		//if((content[content_length - 1] != '\n'))
			printf("\n");
	}

	ret = smcp_outbound_send();
	free((void*)path);
bail:
	return ret;
}

int
main(void) {
	smcp_t instance = smcp_create(0);

	if(!instance) {
		perror("Unable to create SMCP instance");
		abort();
	}

	printf("Listening on port %d\n",smcp_get_port(instance));

	smcp_set_default_request_handler(instance, &request_handler, NULL);

	while(1) {
		smcp_process(instance);
	}

	smcp_release(instance);

	return 0;
}



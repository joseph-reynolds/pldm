#include "file_io.h"
#include <endian.h>
#include <string.h>

int decode_rw_file_memory_req(const uint8_t *msg, size_t payload_length,
			      uint32_t *file_handle, uint32_t *offset,
			      uint32_t *length, uint64_t *address)
{
	if (msg == NULL || file_handle == NULL || offset == NULL ||
	    length == NULL || address == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_RW_FILE_MEM_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	*file_handle = le32toh(*((uint32_t *)msg));
	*offset = le32toh(*((uint32_t *)(msg + sizeof(*file_handle))));
	*length = le32toh(
	    *((uint32_t *)(msg + sizeof(*file_handle) + sizeof(*offset))));
	*address = le64toh(*((uint64_t *)(msg + sizeof(*file_handle) +
					  sizeof(*offset) + sizeof(*length))));

	return PLDM_SUCCESS;
}

int encode_rw_file_memory_resp(uint8_t instance_id, uint8_t command,
			       uint8_t completion_code, uint32_t length,
			       struct pldm_msg *msg)
{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	uint8_t *payload = msg->payload;
	*payload = completion_code;

	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_OEM;
	header.command = command;

	if ((rc = pack_pldm_header(&header, &(msg->hdr))) > PLDM_SUCCESS) {
		return rc;
	}

	if (msg->payload[0] == PLDM_SUCCESS) {
		uint8_t *dst = msg->payload + sizeof(completion_code);
		length = htole32(length);
		memcpy(dst, &length, sizeof(length));
	}

	return PLDM_SUCCESS;
}

int decode_get_file_table_req(const uint8_t *msg, size_t payload_length,
			      uint32_t *transfer_handle,
			      uint8_t *transfer_opflag, uint8_t *table_type)
{
	if (msg == NULL || transfer_handle == NULL || transfer_opflag == NULL ||
	    table_type == NULL) {
		return PLDM_ERROR_INVALID_DATA;
	}

	if (payload_length != PLDM_GET_FILE_TABLE_REQ_BYTES) {
		return PLDM_ERROR_INVALID_LENGTH;
	}

	struct pldm_get_file_table_req *request =
	    (struct pldm_get_file_table_req *)msg;

	*transfer_handle = le32toh(request->transfer_handle);
	*transfer_opflag = request->operation_flag;
	*table_type = request->table_type;

	return PLDM_SUCCESS;
}

int encode_get_file_table_resp(uint8_t instance_id, uint8_t completion_code,
			       uint32_t next_transfer_handle,
			       uint8_t transfer_flag, const uint8_t *table_data,
			       size_t table_size, struct pldm_msg *msg)
{
	struct pldm_header_info header = {0};
	int rc = PLDM_SUCCESS;

	header.msg_type = PLDM_RESPONSE;
	header.instance = instance_id;
	header.pldm_type = PLDM_OEM;
	header.command = PLDM_GET_FILE_TABLE;

	if ((rc = pack_pldm_header(&header, &(msg->hdr))) > PLDM_SUCCESS) {
		return rc;
	}

	struct pldm_get_file_table_resp *response =
	    (struct pldm_get_file_table_resp *)msg->payload;
	response->completion_code = completion_code;

	if (completion_code == PLDM_SUCCESS) {
		response->next_transfer_handle = htole32(next_transfer_handle);
		response->transfer_flag = transfer_flag;
		memcpy(response->table_data, table_data, table_size);
	}

	return PLDM_SUCCESS;
}

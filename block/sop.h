#ifndef _SOP_H
#define _SOP_H
/*
 *    SCSI over PCI (SOP) driver
 *    Copyright 2012 Hewlett-Packard Development Company, L.P.
 *    Copyright 2012 SanDisk Inc.
 *
 *    This program is licensed under the GNU General Public License
 *    version 2
 *
 *    This program is distributed "as is" and WITHOUT ANY WARRANTY
 *    of any kind whatsoever, including without limitation the implied
 *    warranty of MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *    Please see the GNU General Public License v.2 at
 *    http://www.gnu.org/licenses/licenses.en.html for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *    Questions/Comments/Bugfixes to iss_storagedev@hp.com
 *
 */

#define MAX_SGLS	(128)
#define MAX_IO_CMDS	(2048)
#define MAX_ADMIN_CMDS	(64)
#define MAX_CMDS	(1024)
#define MAX_CMDS_LOW	(64)

/* #define	SOP_SUPPORT_BIO_LOG	1 */
/* #define	SOP_IO_COUNTERS */

struct sop_request;

#define	SOP_SIGNATURE_STR	"PQI DREG"
#pragma pack(1)
/*
 * A note about variable names:
 *
 * "iq" == "inbound queue"
 * "oq" == "outbound queue"
 * "pi" == "producer index"
 * "ci" == "consumer index"
 *
 * "inbound" and "outbound" are from the point of view of the device,
 * so "inbound" means "from the host to the device" and "outbound"
 * means "from the device to the host".
 *
 */

struct pqi_device_register_set {
	u64 signature;
	u64 process_admin_function;
#define PQI_IDLE 0
#define PQI_CREATE_ADMIN_QUEUES 0x01ULL
#define PQI_DELETE_ADMIN_QUEUES 0x02ULL
	u64 capability;
	u32 legacy_intx_status;
	u32 legacy_intx_mask_set;
	u32 legacy_intx_mask_clear;
	u8  reserved1[28];
	u32 pqi_device_status;
#define PQI_READY_FOR_ADMIN_FUNCTION 0x02
#define PQI_READY_FOR_IO 0x03
#define PQI_ERROR 0x04
	u8 reserved2[4];
	u64 admin_iq_pi_offset;
	u64 admin_oq_ci_offset;
	u64 admin_iq_addr;
	u64 admin_oq_addr;
	u64 admin_iq_ci_addr;
	u64 admin_oq_pi_addr;
	u32 admin_queue_param;
	u8  reserved3[4];
	u64 device_error;
	u64 error_data;
	u32 reset;
	u32 power_action;
};
#pragma pack()

struct pqi_device_queue {
	void *vaddr;

	/*
	 * For to-device queues, the producer index (pi) is a register on the
	 * device, and the consumer index (ci) is volatile in host memory.
	 * For from-device queues, the producer index is volatile in host
	 * memory, and the consumer index is a register on the device.
	 */
	union {
		struct {
			__iomem u16 *pi;
			volatile u16 *ci;
		} to_dev;
		struct {
			volatile u16 *pi;
			__iomem u16 *ci;
		} from_dev;
	} index;

	u16 unposted_index;		/* temporary host copy of pi or ci,
					 * depending on direction of queue.
					 */
	u16 local_pi;			/* local copy of what was last
					 * written to *pi for inbound queues
					 */
	u16 element_size;		/* must be multiple of 16 */
	u16 nelements;
	u16 queue_id;
	u8 direction;
	spinlock_t index_lock;
#define PQI_DIR_TO_DEVICE 0
#define PQI_DIR_FROM_DEVICE 1
	struct sop_request *cur_req; /* used by oq only */
	struct pqi_device_register_set *registers;
	spinlock_t qlock;
	dma_addr_t dhandle;
};

#define PQI_QUEUE_FULL (-1)
#define PQI_QUEUE_EMPTY (-2)

#define	PQI_ADMIN_INTR	0
#define PQI_IOQ_INTR	2


#pragma pack(1)

struct pqi_oq_extra_params {
	u16 interrupt_message_number;
	u16 wait_for_rearm;
	u16 coalesce_count;
	u16 min_coalesce_time;
	u16 max_coalesce_time;
	u8 operational_queue_protocol;
};

struct pqi_iq_extra_params {
	u8 operational_queue_protocol;
	u8 reserved[10];
};

#define PQI_IU_HEADER_SIZE	4

struct pqi_create_operational_queue_request {
	u8 iu_type;
#define OPERATIONAL_QUEUE_IU_TYPE 0x60
	u8 compatible_features;
	u16 iu_length;
	u16 response_oq;
	u16 work_area;
	u16 request_id;
	u8 function_code;
#define CREATE_QUEUE_TO_DEVICE 0x10
#define CREATE_QUEUE_FROM_DEVICE 0x11
#define DELETE_QUEUE_TO_DEVICE 0x12
#define DELETE_QUEUE_FROM_DEVICE 0x13
	u8 reserved2;
	u16 queue_id;
	u8 reserved3[2];
	u64 element_array_addr;
	u64 index_addr;
	u16 nelements;
	u16 element_length;
	union {
		struct pqi_iq_extra_params iqp;
		struct pqi_oq_extra_params oqp;
	};
	u8 reserved4[17];
};

struct pqi_create_operational_queue_response {
	u8 iu_type;
#define ADMIN_RESPONSE_IU_TYPE		0xE0
	u8 compatible_features;
	u16 iu_length;
	u16 response_oq;
	u16 work_area;
	u16 request_id;
	u8 function_code;
#define DATA_BUFFER_OK 0x00
#define DATA_BUFFER_UNDERFLOW 0x01
#define DATA_BUFFER_ERROR 0x40
#define DATA_BUFFER_OVERFLOW 0x41
#define PCIE_FABRIC_ERROR 0x60
#define PCIE_COMPLETION_TIMEOUT 0x61
#define PCIE_COMPLETER_ABORT 0x62
#define PCIE_POISONED_TLB_RECEIVED 0x63
#define PCIE_ECRC_CHECK_FAILED 0x64
#define PCIE_UNSUPPORTED_REQUEST 0x65
#define PCIE_ACS_VIOLATION 0x66
#define PCIE_TLP_PREFIX_BLOCKED 0x67
	u8 status;
	u8 reserved2[4];
	u64 index_offset;
	u8 reserved3[40];
};

struct pqi_delete_operational_queue_request {
	u8 iu_type;
	u8 compatible_features;
	u16 iu_length;
	u16 response_oq;
	u16 work_area;
	u16 request_id;
	u8 function_code;
	u8 reserved2;
	u16 queue_id;
	u8 reserved3[50];
};

struct pqi_delete_operational_queue_response {
	u8 iu_type;
	u8 compatible_features;
	u16 iu_length;
	u16 response_oq;
	u16 work_area;
	u16 request_id;
	u8 function_code;
	u8 status;
	u8 reserved2[52];
};

struct pqi_capability {
	u8 max_admin_iq_elements;
	u8 max_admin_oq_elements;
	u8 admin_iq_element_length; /* length in 16 byte units */
	u8 admin_oq_element_length; /* length in 16 byte units */
	u8 reserved[4];
};

struct pqi_sgl_descriptor {
	u64 address;
	u32 length;
	u8 reserved[3];
	u8 descriptor_type;
#define PQI_SGL_DATA_BLOCK (0x00 << 4)
#define PQI_SGL_BIT_BUCKET (0x01 << 4)
#define PQI_SGL_STANDARD_SEG (0x02 << 4)
#define PQI_SGL_STANDARD_LAST_SEG (0x03 << 4)
};

struct report_pqi_device_capability_iu {
	u8 iu_type;
#define REPORT_PQI_DEVICE_CAPABILITY 0x60
	u8 compatible_features;
	u16 iu_length;
	u16 response_oq;
	u16 work_area;
	u16 request_id;
	u8 function_code;
	u8 reserved[33];
	u32 buffer_size;
	struct pqi_sgl_descriptor sg;
};

struct report_pqi_device_capability_response {
	u8 iu_type;
#define REPORT_PQI_DEVICE_CAPABILITY_RESPONSE 0xE0
	u8 compatible_features;
	u16 iu_length;
	u16 queue_id;
	u16 work_area;
	u16 request_id;
	u8 function_code;
	u8 status;
	u32 additional_status;
	u8 reserved[63-15];
};

struct pqi_iu_layer_descriptor {
	u8 inbound_spanning;
	u8 reserved1[5];
	u16 max_inbound_iu_len;
	u8 outbound_spanning;
	u8 reserved2[5];
	u16 max_outbound_iu_len;
};

struct pqi_device_capabilities {
	u16 length;
	u8 reserved[14];
	u16 max_iqs;
	u16 max_iq_elements;
	u8 reserved2[4];
	u16 max_iq_element_length;
	u16 min_iq_element_length;
	u8 reserved3[2];
	u16 max_oqs;
	u16 max_oq_elements;
	u16 intr_coalescing_time_granularity;
	u16 max_oq_element_length;
	u16 min_oq_element_length;
	u8 reserved4[4];
	u32 protocol_support_bitmask;
	u16 admin_sgl_support_bitmask;
	u8 reserved5[63 - 49];
	struct pqi_iu_layer_descriptor iu_desc[32];
};
#pragma pack()
#define SOP_MINORS 64

#define IQ_IU_SIZE 64
#define OQ_IU_SIZE 64
#define DRIVER_MAX_IQ_NELEMENTS MAX_CMDS
#define DRIVER_MAX_OQ_NELEMENTS MAX_CMDS

/* Note: Value below is stored in u16 */
#define	MAX_SOP_TIMEOUT		64
#define	DEF_IO_TIMEOUT		30
#define INVALID_TIMEOUT		0xFFFF

#define MAX_RETRY_COUNT		3

struct sop_timeout {
	atomic_t time_slot[MAX_SOP_TIMEOUT];
	atomic_t cur_slot;
};

struct sop_device;
struct pqi_sgl_descriptor;
struct queue_info {
	struct sop_device *h;
	int msix_entry;
	int msix_vector;
	u16 qdepth;
	u16 numa_node;
#ifdef SOP_SUPPORT_BIO_LOG
	u32 seq_num;
#endif
	atomic_t cur_qdepth;
	u32 max_qdepth;
	u32 waitlist_depth;
	struct pqi_device_queue *iq;
	struct pqi_device_queue *oq;
	struct bio_list wait_list;
	struct sop_timeout tmo;
};

struct pqi_device_capability_info {
	/* cached data about PQI device limits */
	u16 max_iqs;
	u16 max_iq_elements;
	u16 max_iq_element_length;
	u16 min_iq_element_length;
	u16 max_oqs;
	u16 max_oq_elements;
	u16 max_oq_element_length;
	u16 min_oq_element_length;
	u16 intr_coalescing_time_granularity;
	u32 protocol_support_bitmask;
	u16 admin_sgl_support_bitmask;
};

struct sop_request_pool {
	struct sop_request *request;
	unsigned long *request_bits;
	u16 num_requests;
	u16 numa_node;
	struct pqi_sgl_descriptor *sg;
	dma_addr_t sg_bus_addr;
};

struct sop_device {
	struct list_head node;
	struct pci_dev *pdev;
	struct pqi_capability pqicap;
	__iomem struct pqi_device_register_set *pqireg;
#define SOP_FLAGS_BITPOS_DO_RESET	0
#define SOP_FLAGS_BITPOS_DO_REM		1
#define SOP_FLAGS_BITPOS_RESET_PEND	2
#define SOP_FLAGS_BITPOS_ADMIN_RDY	8
#define SOP_FLAGS_BITPOS_IOQ_RDY	9
#define SOP_FLAGS_BITPOS_REVALIDATE	10
#define SOP_FLAGS_MASK_DO_RESET		(1 << SOP_FLAGS_BITPOS_DO_RESET)
#define SOP_FLAGS_MASK_DO_REM		(1 << SOP_FLAGS_BITPOS_DO_REM)
#define SOP_FLAGS_MASK_RESET_PEND	(1 << SOP_FLAGS_BITPOS_RESET_PEND)
#define SOP_FLAGS_MASK_ADMIN_RDY	(1 << SOP_FLAGS_BITPOS_ADMIN_RDY)
#define SOP_FLAGS_MASK_IOQ_RDY		(1 << SOP_FLAGS_BITPOS_IOQ_RDY)
#define SOP_FLAGS_MASK_REVALIDATE	(1 << SOP_FLAGS_BITPOS_REVALIDATE)
	unsigned long flags;
#define MAX_IO_QUEUE_PAIRS 32
#define MAX_TOTAL_QUEUE_PAIRS (MAX_IO_QUEUE_PAIRS + 1)
	int nr_queue_pairs; /* total number of *pairs* of queues */
#define INTR_MODE_MSIX 1
#define INTR_MODE_MSI  2
#define INTR_MODE_INTX 3
	int intr_mode;
#define	SOP_MAXNAME_LEN	16
	char devname[SOP_MAXNAME_LEN];
	int ctlr;
	struct pqi_device_queue *io_q_to_dev;
	struct pqi_device_queue *io_q_from_dev;
	u16 current_id;

	/* Command counters */
	atomic_t bio_count;
	atomic_t cmd_pending;
	u32  max_cmd_pending;

#ifdef SOP_IO_COUNTERS
	/* Cumulative counters */
#define	SOP_PERF_BLOCK_SIZE_LIMIT	4096

	atomic_t total_sgio_count;
	atomic_t total_low_io_count;
	atomic_t total_hi_io_count;
	u32  max_io_size;
	u32  min_io_size;
	atomic_t total_max_size_count;
	atomic_t total_min_size_count;
#endif /* SOP_IO_COUNTERS */

	struct queue_info qinfo[MAX_TOTAL_QUEUE_PAIRS];
#define qpindex_from_pqiq(pqiq) (pqiq->queue_id)
/* TODO probably do not need this - calculate from qinfo address */
#define qinfo_to_qid(qinfo) (qpindex_from_pqiq(qinfo->oq))
#define qpindex_to_qid(qpindex, to_device) (qpindex)
	int instance;
	struct delayed_work dwork;
	sector_t capacity;
	int block_size;
	struct request_queue *rq;
	struct gendisk *disk;
	u32 max_hw_sectors;
	int elements_per_io_queue;
	int max_sgls;
	struct pqi_device_capability_info devcap;

	/* Next two fields are for dealing with REQ_FLUSH with data
	 * We can get away with just enough to track only a single such
	 * bio because there can only be one such bio active
	 * at a time (see "C1" comment in block/blk-flush.c 
	 */
	struct bio *req_flush_bio;
	int sync_cache_done;

	struct sop_request_pool admin_req;
	struct sop_request_pool *io_req;
	int num_io_req_pool;
};

#define	SOP_DEVICE_BUSY(_h)	(((_h)->flags) & (\
					SOP_FLAGS_MASK_DO_RESET | \
					SOP_FLAGS_MASK_RESET_PEND))
#define	SOP_DEVICE_REM(_h)	(((_h)->flags) & SOP_FLAGS_MASK_DO_REM)
#define	SOP_DEVICE_READY(_h)	(((_h)->flags & SOP_FLAGS_MASK_ADMIN_RDY) && \
					((_h)->flags & SOP_FLAGS_MASK_IOQ_RDY))
#define	SOP_REVALIDATE(_h)	((_h)->flags & SOP_FLAGS_MASK_REVALIDATE)

#pragma pack()

#define MAX_RESPONSE_SIZE 64
struct sop_request {
	struct completion *waiting;
	struct bio *bio;
	struct scatterlist *sgl;
	u32 xfer_size;
	u16 response_accumulated;
	u16 request_id;
	u8 num_sg;
	u8 retry_count;
	u16 tmo_slot;
	u16 qid;
	u16 log_index;		/* Used for log only - reserved otherwise */
	unsigned long start_time;
	u8 response[MAX_RESPONSE_SIZE];
};

struct sop_sync_cdb_req {
	/* input parameters */
	unsigned char cdb[16];
	u8 cdblen;
	u8 timeout;
	struct iovec *iov;
	int iov_count;
	int data_len;
	int data_dir;

	/* field for partial issue */
	int iovec_idx;

	/* return value */
	u8 scsi_status;
	u8 sense_key;
	u16 sense_asc_ascq;
	sg_io_hdr_t *sg_hdr;
};

#pragma pack(1)
struct sop_limited_cmd_iu {
	u8 iu_type;
#define SOP_LIMITED_CMD_IU	0x10
	u8 compatible_features;
	u16 iu_length;
	u16 queue_id;
	u16 work_area;
	u16 request_id;
	u8 flags;
#define SOP_DATA_DIR_NONE		0x00
#define SOP_DATA_DIR_FROM_DEVICE	0x01
#define SOP_DATA_DIR_TO_DEVICE		0x02
#define SOP_DATA_DIR_RESERVED		0x03
#define SOP_PARTIAL_DATA_BUFFER		0x04
	u8 reserved;
	u32 xfer_size;
	u8 cdb[16];
	struct pqi_sgl_descriptor sg[2];
};
#pragma pack()

#define SOP_RESPONSE_CMD_SUCCESS_IU_TYPE 0x90
#define SOP_RESPONSE_CMD_RESPONSE_IU_TYPE 0x91
#define SOP_RESPONSE_TASK_MGMT_RESPONSE_IU_TYPE 0x93
#define SOP_RESPONSE_TASK_MGMT_RESPONSE_IU_TYPE 0x93

#define	SOP_RESPONSE_INTERNAL_CMD_FAIL_IU_TYPE	0xE8
#define	SOP_RESPONSE_TIMEOUT_CMD_FAIL_IU_TYPE	0xE9

#pragma pack(1)
struct sop_cmd_response {
	u8 iu_type;
	u8 compatible_features;
	u16 iu_length;
	u16 queue_id;
	u16 work_area;
	u16 request_id;
	u16 nexus_id;
	u8 data_in_xfer_result;
	u8 data_out_xfer_result;
	u8 reserved[3];
	u8 status;
	u16 status_qualifier;
	u16 sense_data_len;
	u16 response_data_len;
	u32 data_in_xferred;
	u32 data_out_xferred;
	union {
		u8 response[0];
		u8 sense[0];
	};
};
#pragma pack()

#pragma pack(1)
struct sop_cmd_ui {
	u8 iu_type;
#define SOP_CMD_IU	0x11
	u8 compatible_features;
	u16 iu_length;
	u16 queue_id;
	u16 work_area;
	u16 request_id;
	u16 nexus_id;
	u32 xfer_size;
	u64 lun;
	u16 protocol_specific;
	u8 flags;
	u8 reserved[3];
	u8 priority_task_attr;
	u8 additional_cdb_bytes;
	u8 cdb[16];
	/* total size is 64 bytes, sgl follows in next IU. */
};
#pragma pack()

#pragma pack(1)
struct sop_task_mgmt_iu {
	u8 iu_type;
#define SOP_TASK_MGMT_IU	0x13
	u8 compatible_features;
	u16 iu_length;
	u16 queue_id;
	u16 work_area;
	u16 request_id;
	u16 nexus_id;
	u32 reserved;
	u64 lun;
	u16 protocol_specific;
	u16 reserved2;
	u16 request_id_to_manage;
	u8 task_mgmt_function;
#define SOP_ABORT_TASK 0x01
#define SOP_ABORT_TASK_SET 0x02
#define SOP_CLEAR_TASK_SET 0x04
#define SOP_LUN_RESET 0x08
#define SOP_I_T_NEXUS_RESET 0x10
#define SOP_CLEAR_ACA 0x40
#define SOP_QUERY_TASK 0x80
#define SOP_QUERY_TASK_SET 0x81
#define SOP_QUERY_ASYNC_EVENT 0x82
	u8 reserved3;
};
#pragma pack()

#pragma pack(1)
struct sop_task_mgmt_response {
	u8 iu_type;
	u8 compatible_features;
	u16 iu_length;
	u16 queue_id;
	u16 work_area;
	u16 request_id;
	u16 nexus_id;
	u8 additional_response_info[3];
	u8 response_code;
#define SOP_TMF_COMPLETE 0x00
#define SOP_TMF_REJECTED 0x04
#define SOP_TMF_FAILED 0x05
#define SOP_TMF_SUCCEEDED 0x08
#define SOP_INCORRECT_LUN 0x09
#define SOP_OVERLAPPED_REQUEST_ID_ATTEMPTED 0x0A
#define SOP_INVALID_IU_TYPE 0x20
#define SOP_INVALID_IU_LENGTH 0x21
#define SOP_INVALID_LENGTH_IN_IU 0x22
#define SOP_MISALIGNED_LENGTH_IN_IU 0x23
#define SOP_INVALID_FIELD_IN_IU 0x24
#define SOP_IU_TOO_LONG 0x25

};
#pragma pack()

#pragma pack(1)
struct report_general_iu {
	u8 iu_type;
#define REPORT_GENERAL_IU 0x01
	u8 compatible_features;
	u16 iu_length;
	u16 response_oq;
	u16 work_area;
	u16 request_id;
	u16 reserved;
	u32 buffer_size;
	u8 reserved2[16];
	struct pqi_sgl_descriptor sg;
};
#pragma pack()

#pragma pack(1)
struct report_general_response {
	u8 reserved[4];
	u8 lun_bridge_present_flags;
#define SOP_TARGET_BRIDGE_PRESENT 0x01
#define LOGICAL_UNITS_PRESENT 0x02
	u8 reserved2[3];
	u8 app_clients_present_flags;
#define SOP_APPLICATION_CLIENTS_PRESENT 0x02
	u8 reserved3[9];
	u16 max_incoming_iu_size;
	u16 max_incoming_embedded_data_buffers;
	u16 max_data_buffers;
	u8 reserved4[8];
	u8 incoming_iu_type_support_bitmask[32];
	u8 vendor_specific[8];
	u8 reserved5[2];
	u16 queuing_layer_specific_data_len;
	u16 incoming_sgl_support_bitmask;
	u8 reserved6[2];
};
#pragma pack()

#pragma pack(1)
struct management_response_iu {
	u8 iu_type;
#define MANAGEMENT_RESPONSE_IU 0x81
	u8 compatible_features;
	u16 iu_length;
	u16 queue_id;
	u16 work_area;
	u16 request_id;
	u8 result;
#define MGMT_RSP_RSLT_GOOD 0
#define MGMT_RSP_RSLT_UNKNOWN_ERROR 0x40
#define MGMT_RSP_RSLT_INVALID_FIELD_IN_REQUEST_IU 0x41
#define MGMT_RSP_RSLT_INVALID_FIELD_IN_DATA_OUT_BUFFER 0x42
#define MGMT_RSP_RSLT_VENDOR_SPECIFIC_ERROR 0x7f
#define MGMT_RSP_RSLT_VENDOR_SPECIFIC_ERROR2 0xff
	u8 reserved[5];
	union {
		struct {
			u16 byte_ptr;
			u8 reserved;
			u8 bit_ptr;
		} invalid_field_in_request_ui;
		struct {
			u32 byte_ptr;
			u8 bit_ptr;
		} invalid_field_in_data_out;
	};
};
#pragma pack()

#endif

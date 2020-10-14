
#include <stdio.h>
#include <unistd.h>

#include <rte_malloc.h>
#include <rte_mbuf_pool_ops.h>
#include <rte_random.h>
#include <rte_eal.h>
#include <rte_cryptodev.h>

#ifdef RTE_LIBRTE_PMD_CRYPTO_SCHEDULER
#include <rte_cryptodev_scheduler.h>
#endif

#include "data_vectors.h"
#include "dpdk_cryptodev_client.h"

rte_crypto_cipher_algorithm crypto_cipher_algo_map[CRYPTO_CIPHER_ALGO_LAST];
rte_crypto_cipher_operation crypto_cipher_op_map[CRYPTO_CIPHER_OP_LAST];

enum { MAX_SESS_NUM = 2 * 2 * 4000 };

int Dpdk_cryptodev_client::fill_session_pool_socket(int32_t socket_id, uint32_t session_priv_size, uint32_t nb_sessions)
{
	char mp_name[RTE_MEMPOOL_NAMESIZE];
	struct rte_mempool *sess_mp;

	if (_priv_mp == NULL) {
		snprintf(mp_name, RTE_MEMPOOL_NAMESIZE, "priv_sess_mp_%u", socket_id);

		sess_mp = rte_mempool_create(mp_name,
					nb_sessions,
					session_priv_size,
					0, 
					0, 
					NULL, 
					NULL, 
					NULL,
					NULL, 
					socket_id,
					0);

		if (sess_mp == NULL) {
			printf("Cannot create pool \"%s\" on socket %d\n",mp_name, socket_id);
			return -ENOMEM;
		}

		printf("Allocated pool \"%s\" on socket %d\n", mp_name, socket_id);

		_priv_mp = sess_mp;
	}

	if (_sess_mp == NULL) {

		snprintf(mp_name, RTE_MEMPOOL_NAMESIZE, "sess_mp_%u", socket_id);

		sess_mp = rte_cryptodev_sym_session_pool_create(mp_name,
					nb_sessions, 0, 0, 0, socket_id);

		if (sess_mp == NULL) {
			printf("Cannot create pool \"%s\" on socket %d\n", mp_name, socket_id);
			return -ENOMEM;
		}

		printf("Allocated pool \"%s\" on socket %d\n", mp_name, socket_id);

		_sess_mp = sess_mp;
	}

	return 0;
}

int Dpdk_cryptodev_client::init_inner()
{
	uint8_t enabled_cdev_count = 0, nb_lcores, cdev_id;
	uint32_t sessions_needed = 0;
	unsigned int i, j;
	int ret;

	enabled_cdev_count = rte_cryptodev_devices_get(_opts.device_type, _enabled_cdevs, RTE_CRYPTO_MAX_DEVS);
	printf("enabled_cdev_count: %d\n", enabled_cdev_count);
	if (enabled_cdev_count == 0) 
	{
		printf("No crypto devices type %s available\n", _opts.device_type);
		return -EINVAL;
	}
	printf("enabled_cdev[0] id: %d\n", _enabled_cdevs[0]);
	
	nb_lcores = 1;

	// Create a mempool shared by all the devices 
	uint32_t max_sess_size = 0, sess_size;

	for (cdev_id = 0; cdev_id < rte_cryptodev_count(); cdev_id++) 
	{
		sess_size = rte_cryptodev_sym_get_private_session_size(cdev_id);
		if (sess_size > max_sess_size)
			max_sess_size = sess_size;
	}

	// number of queue pairs
	_opts.nb_qps = 1;
	
	for (i = 0; i < enabled_cdev_count && i < RTE_CRYPTO_MAX_DEVS; i++) 
	{
		cdev_id = _enabled_cdevs[i];
#ifdef RTE_LIBRTE_PMD_CRYPTO_SCHEDULER
		/*
		 * If multi-core scheduler is used, limit the number
		 * of queue pairs to 1, as there is no way to know
		 * how many cores are being used by the PMD, and
		 * how many will be available for the application.
		 */
		if (!strcmp((const char *)_opts.device_type, "crypto_scheduler") &&
				rte_cryptodev_scheduler_mode_get(cdev_id) ==
				CDEV_SCHED_MODE_MULTICORE)
			_opts.nb_qps = 1;
#endif
		struct rte_cryptodev_info cdev_info;
		uint8_t socket_id = rte_cryptodev_socket_id(cdev_id);

		if (socket_id >= RTE_MAX_NUMA_NODES)
			socket_id = 0;

		rte_cryptodev_info_get(cdev_id, &cdev_info);
		if (_opts.nb_qps > cdev_info.max_nb_queue_pairs) {
			printf("Number of needed queue pairs is higher than the maximum number of queue pairs per device.\n");
			printf("Lower the number of cores or increase the number of crypto devices\n");
			return -EINVAL;
		}
		struct rte_cryptodev_config conf;
        
		conf.nb_queue_pairs = _opts.nb_qps;
		conf.socket_id = socket_id;
		conf.ff_disable = RTE_CRYPTODEV_FF_SECURITY | RTE_CRYPTODEV_FF_ASYMMETRIC_CRYPTO;

		struct rte_cryptodev_qp_conf qp_conf;
		qp_conf.nb_descriptors = _opts.nb_descriptors;

		/**
		 * Device info specifies the min headroom and tailroom
		 * requirement for the crypto PMD. This need to be honoured
		 * by the application, while creating mbuf.
		 */
		if (_opts.headroom_sz < cdev_info.min_mbuf_headroom_req) 
		{
			_opts.headroom_sz = cdev_info.min_mbuf_headroom_req;
		}
		if (_opts.tailroom_sz < cdev_info.min_mbuf_tailroom_req) 
		{
			_opts.tailroom_sz = cdev_info.min_mbuf_tailroom_req;
		}

		// Update segment size to include headroom & tailroom
		_opts.segment_sz += (_opts.headroom_sz + _opts.tailroom_sz);

		uint32_t dev_max_nb_sess = cdev_info.sym.max_nb_sessions;
		printf("dev_max_nb_sess: %u\n", dev_max_nb_sess);

		sessions_needed = MAX_SESS_NUM;
		// Done only onece - per all devices - review?
		ret = fill_session_pool_socket(socket_id, max_sess_size, sessions_needed);
		if (ret < 0)
			return ret;

		qp_conf.mp_session = _sess_mp;
		qp_conf.mp_session_private = _priv_mp;

		ret = rte_cryptodev_configure(cdev_id, &conf);
		if (ret < 0) 
		{
			printf("Failed to configure cryptodev %u", cdev_id);
			return -EINVAL;
		}

		for (j = 0; j < _opts.nb_qps; j++) 
		{
			ret = rte_cryptodev_queue_pair_setup(cdev_id, j, &qp_conf, socket_id);
			if (ret < 0) 
			{
				printf("Failed to setup queue pair %u on cryptodev %u",	j, cdev_id);
				return -EINVAL;
			}
		}

		if (alloc_common_memory(
			cdev_id, 
			0, //qp_id, 
			0 // extra_priv
			) < 0)
		{
			printf("Failed to alloc_common_memory\n");
			return -EPERM;
		}

		ret = rte_cryptodev_start(cdev_id);
		if (ret < 0) 
		{
			printf("Failed to start device %u: error %d\n", cdev_id, ret);
			return -EPERM;
		}
	}

	return enabled_cdev_count;
}

int Dpdk_cryptodev_client::verify_devices_capabilities(struct Dpdk_cryptodev_options *opts)
{
	struct rte_cryptodev_sym_capability_idx cap_idx;
	const struct rte_cryptodev_symmetric_capability *capability;

	uint8_t i, cdev_id;
	int ret;

	for (i = 0; i < _nb_cryptodevs; i++) 
	{
		cdev_id = _enabled_cdevs[i];

		cap_idx.type = RTE_CRYPTO_SYM_XFORM_CIPHER;
		cap_idx.algo.cipher = opts->cipher_algo;

		capability = rte_cryptodev_sym_capability_get(cdev_id, &cap_idx);
		if (capability == NULL)
			return -1;

		ret = rte_cryptodev_sym_capability_check_cipher(
					capability,
					opts->cipher_key_sz,
					opts->cipher_iv_sz);
		if (ret != 0)
			return ret;
	}

	return 0;
}

int Dpdk_cryptodev_client::init(int argc, char **argv)
{
	uint8_t i;

	int ret;
	uint32_t lcore_id;

	// TODO review
	crypto_cipher_algo_map[CRYPTO_CIPHER_AES_CBC] = RTE_CRYPTO_CIPHER_AES_CBC;

	crypto_cipher_op_map[CRYPTO_CIPHER_OP_ENCRYPT] = RTE_CRYPTO_CIPHER_OP_ENCRYPT;
	crypto_cipher_op_map[CRYPTO_CIPHER_OP_DECRYPT] = RTE_CRYPTO_CIPHER_OP_DECRYPT;

	// Initialise DPDK EAL 
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments!\n");

	argc -= ret;
	argv += ret;

	cperf_options_default(&_opts);

	ret = cperf_options_parse(&_opts, argc, argv);
	if (ret) 
	{
		RTE_LOG(ERR, USER1, "Parsing on or more user options failed\n");
		goto err;
	}

	ret = cperf_options_check(&_opts);
	if (ret) {
		RTE_LOG(ERR, USER1, "Checking on or more user options failed\n");
		goto err;
	}

	cperf_options_dump(&_opts);

	_nb_cryptodevs = init_inner();

	if (_nb_cryptodevs < 1) 
	{
		RTE_LOG(ERR, USER1, "Failed to initialise requested crypto device type\n");
		_nb_cryptodevs = 0;
		goto err;
	}

	ret = verify_devices_capabilities(&_opts);
	if (ret) 
	{
		RTE_LOG(ERR, USER1, "Crypto device type does not support capabilities requested\n");
		goto err;
	}

    ////
	t_vecs = (struct Dpdk_cryptodev_data_vector *)rte_malloc(NULL, _opts.max_burst_size * sizeof(struct Dpdk_cryptodev_data_vector), 0);
	if (t_vecs == NULL)
	{
		RTE_LOG(ERR, USER1, "t_vecs alloc failed\n");
		goto err;
	}

	out_data = (uint8_t*) malloc(_opts.max_burst_size * _opts.max_buffer_size);
    if (out_data == NULL)
	{
		RTE_LOG(ERR, USER1, "out_data alloc failed\n");
		goto err;
	}

	memset(_active_sessions_registry, 0, sizeof(_active_sessions_registry));

	return 0;

err:
	i = 0;

	for (i = 0; i < _nb_cryptodevs && i < RTE_CRYPTO_MAX_DEVS; i++)
		rte_cryptodev_stop(_enabled_cdevs[i]);

	// TODO
	//free_test_vector(t_vec, &opts);

	return EXIT_FAILURE;
}

void  Dpdk_cryptodev_client::cleanup()
{
	int i = 0;
	
	for (i = 0; i < _nb_cryptodevs && i < RTE_CRYPTO_MAX_DEVS; i++)
		rte_cryptodev_stop(_enabled_cdevs[i]);
	
	//rte_free(opts.imix_buffer_sizes);

	//free_test_vector(t_vec, &opts);
}

int Dpdk_cryptodev_client::run_jobs(struct Dpdk_cryptodev_data_vector* jobs, uint32_t size)
{
	uint16_t total_nb_qps = 0;
	uint8_t cdev_id;
	uint8_t buffer_size_idx = 0;

	total_nb_qps = 1;

	uint8_t qp_id = 0, cdev_index = 0;
	cdev_id = _enabled_cdevs[cdev_index];

	// TODO review t_vecs setup
	// TODO review
	uint32_t i = 0, j = 0;
	while (i < size)
	{
		if (jobs[i].op._sess_op == SESS_OP_CREATE)
		{
			uint32_t sess_id = -1;
			if (0 == create_session(cdev_id, &jobs[i], &sess_id))
			{
				jobs[i].op._op_status = OP_STATUS_SUCC;
				jobs[i].op._sess_id = sess_id;
			}
			else
			{
				jobs[i].op._op_status = OP_STATUS_FAILED;
				jobs[i].op._sess_id = -1;
			}
		}
		else if (jobs[i].op._sess_op == SESS_OP_CLOSE)
		{
			if (0 == remove_session(cdev_id, &jobs[i]))
				jobs[i].op._op_status = OP_STATUS_SUCC;
			else
				jobs[i].op._op_status = OP_STATUS_FAILED;
		}
		else if (jobs[i].op._sess_op == SESS_OP_ATTACH)
		{
			// init with failde
			jobs[i].op._op_status = OP_STATUS_FAILED;

			t_vecs[j].op._sess_id = jobs[i].op._sess_id;
			t_vecs[j].cipher_buff_list[0].data = jobs[i].cipher_buff_list[0].data;
			t_vecs[j].cipher_buff_list[0].length = _opts.max_buffer_size;
			t_vecs[j].cipher_iv.data = jobs[i].cipher_iv.data;
			t_vecs[j].cipher_iv.length = jobs[i].cipher_iv.length;

			j++;
		}
		else
		{
			printf("WARN!!! unknown sess op %d i %d\n", jobs[i].op._sess_op, i);
		}

		i++;
	}

	_opts.total_ops = j;

	// Get next size from range or list 
	if (_opts.inc_buffer_size != 0)
		_opts.test_buffer_size = _opts.min_buffer_size;
	else
		_opts.test_buffer_size = _opts.buffer_size_list[0];

	while (_opts.test_buffer_size <= _opts.max_buffer_size) 
	{
		run_jobs_inner(cdev_id, qp_id);
		
		/* Get next size from range or list */
		if (_opts.inc_buffer_size != 0)
			_opts.test_buffer_size += _opts.inc_buffer_size;
		else 
		{
			if (++buffer_size_idx == _opts.buffer_size_count)
				break;
			_opts.test_buffer_size = _opts.buffer_size_list[buffer_size_idx];
		}
	}

	i = 0, j = 0;
	while (i < size)
	{
		if (jobs[i].op._sess_op == SESS_OP_CREATE)
		{
			;
		}
		else if (jobs[i].op._sess_op == SESS_OP_CLOSE)
		{
			;
		}
		else if (jobs[i].op._sess_op == SESS_OP_ATTACH)
		{
			jobs[i].op._op_status = OP_STATUS_SUCC;
			jobs[i].op._op_in_buff_list_len = 1;
			jobs[i].cipher_buff_list[0].data = t_vecs[j].cipher_buff_list[0].data;
			jobs[i].cipher_buff_list[0].length = t_vecs[j].cipher_buff_list[0].length;

			j++;
		}
		else
		{
			printf("WARN!!! unknown sess op %d i = %d\n", jobs[i].op._sess_op, i);
		}

		i++;
	}

	return 0;
}

// ops
int Dpdk_cryptodev_client::set_ops_cipher(struct rte_crypto_op **ops,
		const struct Dpdk_cryptodev_data_vector *test_vectors,
		uint32_t nb_ops)
{
	uint16_t i;

	uint8_t cdev_id = 0;

	uint16_t iv_offset = sizeof(struct rte_crypto_op) + sizeof(struct rte_crypto_sym_op);

	for (i = 0; i < nb_ops; i++) {
		struct rte_crypto_sym_op *sym_op = ops[i]->sym;

		ops[i]->status = RTE_CRYPTO_OP_STATUS_NOT_PROCESSED;

		static int count = 0;
		ops[i]->sess_type =  RTE_CRYPTO_OP_WITH_SESSION; 

		struct rte_cryptodev_sym_session* sess = get_session(cdev_id, &test_vectors[i]);
		// TODO handle not found sess == NULL
		rte_crypto_op_attach_sym_session(ops[i], sess);

		sym_op->m_src = (struct rte_mbuf *)((uint8_t *)ops[i] + _src_buf_offset);

		// Set dest mbuf to NULL if out-of-place (dst_buf_offset = 0) 
		if (_dst_buf_offset == 0)
			sym_op->m_dst = NULL;
		else
			sym_op->m_dst = (struct rte_mbuf *)((uint8_t *)ops[i] + _dst_buf_offset);

		sym_op->cipher.data.length = _opts.test_buffer_size;
/*
		// TODO cifer algo from session
		if (options->cipher_algo == RTE_CRYPTO_CIPHER_SNOW3G_UEA2 ||
				options->cipher_algo == RTE_CRYPTO_CIPHER_KASUMI_F8 ||
				options->cipher_algo == RTE_CRYPTO_CIPHER_ZUC_EEA3)
			sym_op->cipher.data.length <<= 3;
*/
		sym_op->cipher.data.offset = 0;

		if (test_vectors[i].cipher_iv.length)
		{
			uint8_t *iv_ptr = rte_crypto_op_ctod_offset(ops[i], uint8_t *, iv_offset);
			memcpy(iv_ptr, test_vectors[i].cipher_iv.data, test_vectors[i].cipher_iv.length);
		}
	}

	return 0;
}


int Dpdk_cryptodev_client::create_session(uint8_t dev_id, const struct Dpdk_cryptodev_data_vector *test_vector, uint32_t* sess_id)
{
	uint16_t iv_offset = sizeof(struct rte_crypto_op) + sizeof(struct rte_crypto_sym_op);

	struct rte_crypto_sym_xform cipher_xform;
	struct rte_cryptodev_sym_session *s = NULL;

	s = rte_cryptodev_sym_session_create(_sess_mp);
	if (s == NULL)
		// TODO throw
		return -1;

	cipher_xform.type = RTE_CRYPTO_SYM_XFORM_CIPHER;
	cipher_xform.next = NULL;

	// TODO check index
	cipher_xform.cipher.algo = crypto_cipher_algo_map[test_vector->op._cipher_algo];
	cipher_xform.cipher.op = crypto_cipher_op_map[test_vector->op._cipher_op];
	cipher_xform.cipher.iv.offset = iv_offset;

	printf("%d %d %d\n", cipher_xform.cipher.algo, cipher_xform.cipher.op, cipher_xform.cipher.iv.offset);

	if (cipher_xform.cipher.algo != RTE_CRYPTO_CIPHER_NULL) {
		cipher_xform.cipher.key.data = test_vector->cipher_key.data;
		cipher_xform.cipher.key.length = test_vector->cipher_key.length;
		cipher_xform.cipher.iv.length = test_vector->cipher_iv.length;
	} else {
		cipher_xform.cipher.key.data = NULL;
		cipher_xform.cipher.key.length = 0;
		cipher_xform.cipher.iv.length = 0;
	}
	
    rte_cryptodev_sym_session_init(dev_id, s, &cipher_xform, _priv_mp);
	
	// TODO handle error
	//_active_sessions.insert ( active_sessions_t::value_type(test_vector->sess._sess_id, s) );

	for (uint32_t i = 0; i < MAX_SESS_NUM; i++)
	{
		if (_active_sessions_registry[i] == NULL)
		{
			// find empty slot / assign id
			_active_sessions_registry[i] = s;
			*sess_id = i;

			return 0;
		}
	}

	return -1;
}

int Dpdk_cryptodev_client::remove_session(uint8_t dev_id, const struct Dpdk_cryptodev_data_vector *test_vector)
{
	struct rte_cryptodev_sym_session* s = _active_sessions_registry[test_vector->op._sess_id];
	if (NULL == s)
	{
		RTE_LOG(ERR, USER1, "remove_session s == NULL \n");
		return -1;
	}

	if (0 != rte_cryptodev_sym_session_free(s))
		RTE_LOG(ERR, USER1, "rte_cryptodev_sym_session_free failed\n");
		
	_active_sessions_registry[test_vector->op._sess_id] = NULL;
	
	return 0;
}

struct rte_cryptodev_sym_session* Dpdk_cryptodev_client::get_session(uint8_t dev_id, const struct Dpdk_cryptodev_data_vector *test_vector)
{
	struct rte_cryptodev_sym_session* s = _active_sessions_registry[test_vector->op._sess_id];
	if (NULL == s)
	{
		RTE_LOG(ERR, USER1, "get_session s == NULL \n");
		return NULL;
	}

	return s;
}

// in / out set
//
int Dpdk_cryptodev_client::vec_output_set(struct rte_crypto_op *op,
		struct Dpdk_cryptodev_data_vector *vector,
		// TODO review
		uint8_t* data)
{
	const struct rte_mbuf *m;
	uint32_t len;
	uint16_t nb_segs;

	uint8_t	cipher = 1, auth = 0;
	int res = 0;

	if (op->status != RTE_CRYPTO_OP_STATUS_SUCCESS)
		return 1;

	if (op->sym->m_dst)
		m = op->sym->m_dst;
	else
		m = op->sym->m_src;
	nb_segs = m->nb_segs;
	len = 0;
	while (m && nb_segs != 0) {
		len += m->data_len;
		m = m->next;
		nb_segs--;
	}

	if (op->sym->m_dst)
		m = op->sym->m_dst;
	else
		m = op->sym->m_src;
	nb_segs = m->nb_segs;
	len = 0;
	while (m && nb_segs != 0) {
		memcpy(data + len, rte_pktmbuf_mtod(m, uint8_t *), m->data_len);
		len += m->data_len;
		m = m->next;
		nb_segs--;
	}

	if (cipher == 1) {
		vector->cipher_buff_list[0].data = data;
		vector->cipher_buff_list[0].length = _opts.test_buffer_size; // len?
	}

	return 0;
}

void Dpdk_cryptodev_client::mbuf_set(struct rte_mbuf *mbuf,
		const struct Dpdk_cryptodev_data_vector *test_vector)
{
	uint32_t segment_sz = _opts.segment_sz;
	uint8_t *mbuf_data;
	uint8_t *test_data;
	uint32_t remaining_bytes = _opts.max_buffer_size;

	test_data = test_vector->cipher_buff_list[0].data;

	while (remaining_bytes) {
		mbuf_data = rte_pktmbuf_mtod(mbuf, uint8_t *);

		if (remaining_bytes <= segment_sz) {
			memcpy(mbuf_data, test_data, remaining_bytes);
			return;
		}

		memcpy(mbuf_data, test_data, segment_sz);
		remaining_bytes -= segment_sz;
		test_data += segment_sz;
		mbuf = mbuf->next;
	}
}

//
int Dpdk_cryptodev_client::run_jobs_inner(uint8_t dev_id, uint8_t qp_id)
{
	uint64_t ops_enqd = 0, ops_enqd_total = 0, ops_enqd_failed = 0;
	uint64_t ops_deqd = 0, ops_deqd_total = 0, ops_deqd_failed = 0;
	uint64_t ops_failed = 0;

	static rte_atomic16_t display_once = RTE_ATOMIC16_INIT(0);

	uint64_t i;
	uint16_t ops_unused = 0;

    uint16_t socket_id = 0;

	struct rte_mbuf *mbufs[_opts.max_burst_size];
	struct rte_crypto_op *ops[_opts.max_burst_size];
	struct rte_crypto_op *ops_processed[_opts.max_burst_size];

	uint32_t lcore = rte_lcore_id();

#ifdef CPERF_LINEARIZATION_ENABLE
	struct rte_cryptodev_info dev_info;
	int linearize = 0;

	/* Check if source mbufs require coalescing */
	if (ctx->options->segment_sz < ctx->options->max_buffer_size) {
		rte_cryptodev_info_get(ctx->dev_id, &dev_info);
		if ((dev_info.feature_flags &
				RTE_CRYPTODEV_FF_MBUF_SCATTER_GATHER) == 0)
			linearize = 1;
	}
#endif /* CPERF_LINEARIZATION_ENABLE */

	while (ops_enqd_total < _opts.total_ops) {

		uint16_t burst_size = ((ops_enqd_total + _opts.max_burst_size) <= _opts.total_ops) ?
						_opts.max_burst_size :
						_opts.total_ops - ops_enqd_total;

		uint16_t ops_needed = burst_size - ops_unused;

		if (rte_mempool_get_bulk(_ops_mp, (void **)ops, ops_needed) != 0) 
		{
			RTE_LOG(ERR, USER1,
				"Failed to allocate more crypto operations "
				"from the crypto operation pool.\n"
				"Consider increasing the pool size "
				"with --pool-sz\n");
			return -1;
		}
		
		//populate_ops
        set_ops_cipher(ops, t_vecs, ops_needed);

		// Populate the mbuf with the vector
		for (i = 0; i < ops_needed; i++)
			mbuf_set(ops[i]->sym->m_src, t_vecs + i);

#ifdef CPERF_LINEARIZATION_ENABLE
		if (linearize) {
			/* PMD doesn't support scatter-gather and source buffer
			 * is segmented.
			 * We need to linearize it before enqueuing.
			 */
			for (i = 0; i < burst_size; i++)
				rte_pktmbuf_linearize(ops[i]->sym->m_src);
		}
#endif /* CPERF_LINEARIZATION_ENABLE */

		/* Enqueue burst of ops on crypto device */
		ops_enqd = rte_cryptodev_enqueue_burst(dev_id, qp_id, ops, ops_needed); //burst_size);
		if (ops_enqd < burst_size)
			ops_enqd_failed++;

		/**
		 * Calculate number of ops not enqueued (mainly for hw
		 * accelerators whose ingress queue can fill up).
		 */
		ops_unused = burst_size - ops_enqd;
		ops_enqd_total += ops_enqd;

		/* Dequeue processed burst of ops from crypto device */
		ops_deqd = rte_cryptodev_dequeue_burst(dev_id, qp_id, ops_processed, ops_needed);//ctx->options->max_burst_size);
		if (ops_deqd == 0) {
			/**
			 * Count dequeue polls which didn't return any
			 * processed operations. This statistic is mainly
			 * relevant to hw accelerators.
			 */
			ops_deqd_failed++;
			continue;
		}

		for (i = 0; i < ops_deqd; i++) {
			if (vec_output_set(ops_processed[i],
					t_vecs + i, 
					// TODO review
					out_data + (i* _opts.max_buffer_size)))
				ops_failed++;
		}

		/* Free crypto ops so they can be reused. */
		rte_mempool_put_bulk(_ops_mp, (void **)ops_processed, ops_deqd);
		
		ops_deqd_total += ops_deqd;
	}

	/* Dequeue any operations still in the crypto device */

	while (ops_deqd_total < _opts.total_ops) {
		/* Sending 0 length burst to flush sw crypto device */
		rte_cryptodev_enqueue_burst(dev_id, qp_id, NULL, 0);

		/* dequeue burst */
		ops_deqd = rte_cryptodev_dequeue_burst(dev_id, qp_id, ops_processed, _opts.max_burst_size);
		if (ops_deqd == 0) {
			ops_deqd_failed++;
			continue;
		}

		for (i = 0; i < ops_deqd; i++) {
			if (vec_output_set(ops_processed[i],
					t_vecs + i,
					// TODO review
					out_data + (i* _opts.max_buffer_size)))
				ops_failed++;
		}

		/* Free crypto ops so they can be reused. */
		rte_mempool_put_bulk(_ops_mp, (void **)ops_processed, ops_deqd);
		
		ops_deqd_total += ops_deqd;
	}

	return 0;
}

// TODO to class
/// memory common
struct obj_params {
	uint32_t src_buf_offset;
	uint32_t dst_buf_offset;
	uint16_t segment_sz;
	uint16_t headroom_sz;
	uint16_t data_len;
	uint16_t segments_nb;
};

void fill_single_seg_mbuf(struct rte_mbuf *m, struct rte_mempool *mp,
		void *obj, uint32_t mbuf_offset, uint16_t segment_sz,
		uint16_t headroom, uint16_t data_len)
{
	uint32_t mbuf_hdr_size = sizeof(struct rte_mbuf);

	/* start of buffer is after mbuf structure and priv data */
	m->priv_size = 0;
	m->buf_addr = (char *)m + mbuf_hdr_size;
	m->buf_iova = rte_mempool_virt2iova(obj) + mbuf_offset + mbuf_hdr_size;
	m->buf_len = segment_sz;
	m->data_len = data_len;
	m->pkt_len = data_len;

	/* Use headroom specified for the buffer */
	m->data_off = headroom;

	/* init some constant fields */
	m->pool = mp;
	m->nb_segs = 1;
	m->port = 0xff;
	rte_mbuf_refcnt_set(m, 1);
	m->next = NULL;
}

void fill_multi_seg_mbuf(struct rte_mbuf *m, struct rte_mempool *mp,
		void *obj, uint32_t mbuf_offset, uint16_t segment_sz,
		uint16_t headroom, uint16_t data_len, uint16_t segments_nb)
{
	uint16_t mbuf_hdr_size = sizeof(struct rte_mbuf);
	uint16_t remaining_segments = segments_nb;
	struct rte_mbuf *next_mbuf;
	rte_iova_t next_seg_phys_addr = rte_mempool_virt2iova(obj) + mbuf_offset + mbuf_hdr_size;

	do {
		/* start of buffer is after mbuf structure and priv data */
		m->priv_size = 0;
		m->buf_addr = (char *)m + mbuf_hdr_size;
		m->buf_iova = next_seg_phys_addr;
		next_seg_phys_addr += mbuf_hdr_size + segment_sz;
		m->buf_len = segment_sz;
		m->data_len = data_len;

		/* Use headroom specified for the buffer */
		m->data_off = headroom;

		/* init some constant fields */
		m->pool = mp;
		m->nb_segs = segments_nb;
		m->port = 0xff;
		rte_mbuf_refcnt_set(m, 1);
		next_mbuf = (struct rte_mbuf *) ((uint8_t *) m + mbuf_hdr_size + segment_sz);
		m->next = next_mbuf;
		m = next_mbuf;
		remaining_segments--;

	} while (remaining_segments > 0);

	m->next = NULL;
}

void mempool_obj_init(struct rte_mempool *mp,
		    void *opaque_arg,
		    void *obj,
		    __attribute__((unused)) unsigned int i)
{
	struct obj_params *params = (struct obj_params *) opaque_arg;
	struct rte_crypto_op *op = (struct rte_crypto_op *) obj;
	struct rte_mbuf *m = (struct rte_mbuf *) ((uint8_t *) obj + params->src_buf_offset);
	/* Set crypto operation */
	op->type = RTE_CRYPTO_OP_TYPE_SYMMETRIC;
	op->status = RTE_CRYPTO_OP_STATUS_NOT_PROCESSED;
	op->sess_type =  RTE_CRYPTO_OP_WITH_SESSION;
	op->phys_addr = rte_mem_virt2iova(obj);
	op->mempool = mp;

	/* Set source buffer */
	op->sym->m_src = m;
	if (params->segments_nb == 1)
		fill_single_seg_mbuf(m, mp, obj, params->src_buf_offset,
				params->segment_sz, params->headroom_sz,
				params->data_len);
	else
		fill_multi_seg_mbuf(m, mp, obj, params->src_buf_offset,
				params->segment_sz, params->headroom_sz,
				params->data_len, params->segments_nb);

	/* Set destination buffer */
	if (params->dst_buf_offset) {
		m = (struct rte_mbuf *) ((uint8_t *) obj + params->dst_buf_offset);
		fill_single_seg_mbuf(m, mp, obj, params->dst_buf_offset,
				params->segment_sz, params->headroom_sz,
				params->data_len);
		op->sym->m_dst = m;
	} else
		op->sym->m_dst = NULL;
}

int Dpdk_cryptodev_client::alloc_common_memory(
			uint8_t dev_id, 
			uint16_t qp_id,
			size_t extra_op_priv_size)
{
	const char *mp_ops_name;
	char pool_name[32] = "";
	int ret;

	// Calculate the object size 
	uint16_t crypto_op_size = sizeof(struct rte_crypto_op) + sizeof(struct rte_crypto_sym_op);
	/*
	 * If doing AES-CCM, IV field needs to be 16 bytes long,
	 * and AAD field needs to be long enough to have 18 bytes,
	 * plus the length of the AAD, and all rounded to a
	 * multiple of 16 bytes.
	 */
	uint32_t cipher_iv_length = 16;
	uint16_t crypto_op_private_size = extra_op_priv_size + cipher_iv_length;
//	if (options->aead_algo == RTE_CRYPTO_AEAD_AES_CCM) {
//		crypto_op_private_size = extra_op_priv_size +
//			cipher_iv_length; 
			/*+
			test_vector->auth_iv.length +
			RTE_ALIGN_CEIL(test_vector->aead_iv.length, 16) +
			RTE_ALIGN_CEIL(options->aead_aad_sz + 18, 16);
			*/
//	} else {
//		crypto_op_private_size = extra_op_priv_size +
//			cipher_iv_length;
			/*+
			test_vector->auth_iv.length +
			test_vector->aead_iv.length +
			options->aead_aad_sz;
			*/
//	}

	uint16_t crypto_op_total_size = crypto_op_size + crypto_op_private_size;
	uint16_t crypto_op_total_size_padded = RTE_CACHE_LINE_ROUNDUP(crypto_op_total_size);
	uint32_t mbuf_size = sizeof(struct rte_mbuf) + _opts.segment_sz;
	uint32_t max_size = _opts.max_buffer_size + _opts.digest_sz;
	uint16_t segments_nb = (max_size % _opts.segment_sz) ?
			(max_size / _opts.segment_sz) + 1 :
			max_size / _opts.segment_sz;
	uint32_t obj_size = crypto_op_total_size_padded + (mbuf_size * segments_nb);

	snprintf(pool_name, sizeof(pool_name), "pool_cdev_%u_qp_%u", dev_id, qp_id);

	_src_buf_offset = crypto_op_total_size_padded;

	struct obj_params params;
	params.segment_sz = _opts.segment_sz;
	params.headroom_sz = _opts.headroom_sz;
	/* Data len = segment size - (headroom + tailroom) */
	params.data_len = _opts.segment_sz - _opts.headroom_sz - _opts.tailroom_sz;
	params.segments_nb = segments_nb;
	params.src_buf_offset = crypto_op_total_size_padded;
	params.dst_buf_offset = 0;

	if (_opts.out_of_place) {
		_dst_buf_offset = _src_buf_offset + (mbuf_size * segments_nb);
		params.dst_buf_offset = _dst_buf_offset;
		/* Destination buffer will be one segment only */
		obj_size += max_size;
	}

	_ops_mp = rte_mempool_create_empty(pool_name,
			_opts.pool_sz, 
            obj_size, 
            512, 
            0,
			rte_socket_id(), 
            0);
	if (_ops_mp == NULL) {
		RTE_LOG(ERR, USER1, "Cannot allocate mempool for device %u\n", dev_id);
		return -1;
	}
                                                                                                                                                                                                                                                                                                                                                    
	mp_ops_name = rte_mbuf_best_mempool_ops();

	ret = rte_mempool_set_ops_byname(_ops_mp, mp_ops_name, NULL);
	if (ret != 0) {
		RTE_LOG(ERR, USER1, "Error setting mempool handler for device %u\n", dev_id);
		return -1;
	}

	ret = rte_mempool_populate_default(_ops_mp);
	if (ret < 0) {
		RTE_LOG(ERR, USER1, "Error populating mempool for device %u\n", dev_id);
		return -1;
	}

	rte_mempool_obj_iter(_ops_mp, mempool_obj_init, (void *)&params);

	return 0;
}

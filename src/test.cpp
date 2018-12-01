#include <libwebsockets.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>

int close_testing;
int max_poll_elements;
int debug_level = 7;

volatile int force_exit = 0, dynamic_vhost_enable = 0;
struct lws_vhost *dynamic_vhost;
struct lws_context *context;
static int test_options;

const char *resource_path = "/home/yi/workspace/libwebsockets/yiliu/wstest/www";

#define DUMB_PERIOD_US 50000

struct pss__dumb_increment {
	int number;
};

struct vhd__dumb_increment {
	const unsigned int *options;
};

static int
callback_dumb_increment(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	struct pss__dumb_increment *pss = (struct pss__dumb_increment *)user;
	struct vhd__dumb_increment *vhd =
				(struct vhd__dumb_increment *)
				lws_protocol_vh_priv_get(lws_get_vhost(wsi),
						lws_get_protocol(wsi));
	uint8_t buf[LWS_PRE + 20], *p = &buf[LWS_PRE];
	const struct lws_protocol_vhost_options *opt;
	int n, m;

	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT:
          vhd = (vhd__dumb_increment*)
            lws_protocol_vh_priv_zalloc( lws_get_vhost(wsi),
                                         lws_get_protocol(wsi),
                                         sizeof(struct vhd__dumb_increment));
          if (!vhd)
            return -1;
          if ((opt = lws_pvo_search(
				(const struct lws_protocol_vhost_options *)in,
				"options")))
			vhd->options = (unsigned int *)opt->value;
		break;

	case LWS_CALLBACK_ESTABLISHED:
		pss->number = 0;
		if (!vhd->options || !((*vhd->options) & 1))
			lws_set_timer_usecs(wsi, DUMB_PERIOD_US);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		n = lws_snprintf((char *)p, sizeof(buf) - LWS_PRE, "%d",
				 pss->number++);
		m = lws_write(wsi, p, n, LWS_WRITE_TEXT);
		if (m < n) {
			lwsl_err("ERROR %d writing to di socket\n", n);
			return -1;
		}
		break;

	case LWS_CALLBACK_RECEIVE:
		if (len < 6)
			break;
		if (strncmp((const char *)in, "reset\n", 6) == 0)
			pss->number = 0;
		if (strncmp((const char *)in, "closeme\n", 8) == 0) {
			lwsl_notice("dumb_inc: closing as requested\n");
			lws_close_reason(wsi, LWS_CLOSE_STATUS_GOINGAWAY,
					 (unsigned char *)"seeya", 5);
			return -1;
		}
		break;

	case LWS_CALLBACK_TIMER:
		if (!vhd->options || !((*vhd->options) & 1)) {
			lws_callback_on_writable_all_protocol_vhost(
				lws_get_vhost(wsi), lws_get_protocol(wsi));
			lws_set_timer_usecs(wsi, DUMB_PERIOD_US);
		}
		break;

	default:
		break;
	}

	return 0;
}


static int
lws_callback_http(struct lws *wsi, enum lws_callback_reasons reason, void *user,
		  void *in, size_t len)
{
  const unsigned char *c;
  char buf[1024];
  int n = 0, hlen;
  switch (reason) {
  case LWS_CALLBACK_HTTP:
    /* non-mount-handled accesses will turn up here */
    /* dump the headers */
    do {
      c = lws_token_to_string((lws_token_indexes)n);
      if (!c) {
        n++;
        continue;
      }

      hlen = lws_hdr_total_length(wsi, (lws_token_indexes)n);
      if (!hlen || hlen > (int)sizeof(buf) - 1) {
        n++;
        continue;
      }

      if (lws_hdr_copy(wsi, buf, sizeof buf, (lws_token_indexes)n) < 0)
        fprintf(stderr, "    %s (too big)\n", (char *)c);
      else {
        buf[sizeof(buf) - 1] = '\0';

        fprintf(stderr, "    %s = %s\n", (char *)c, buf);
      }
      n++;
    } while (c);

    /* dump the individual URI Arg parameters */

    n = 0;
    while (lws_hdr_copy_fragment(wsi, buf, sizeof(buf),
                                 WSI_TOKEN_HTTP_URI_ARGS, n) > 0) {
      lwsl_notice("URI Arg %d: %s\n", ++n, buf);
    }

    if (lws_return_http_status(wsi, HTTP_STATUS_NOT_FOUND, NULL))
      return -1;

    if (lws_http_transaction_completed(wsi))
      return -1;

    return 0;
  default:
    break;
  }

  return lws_callback_http_dummy(wsi, reason, user, in, len);
}

/* list of supported protocols and callbacks */



static struct lws_protocols my_protocols[] = {
                                              /* first protocol must always be HTTP handler */
                                              { "http-only", lws_callback_http, 0, 0, },        
                                              	{
                                                  "dumb-increment-protocol",
                                                  callback_dumb_increment,
                                                  sizeof(struct pss__dumb_increment), 
                                                  10, /* rx buf size must be >= permessage-deflate rx size */ 
                                                  0, NULL, 0 
                                                },

                                              { NULL, NULL, 0, 0 } /* terminator */
};


static const struct lws_extension exts[] = {
                                            {
                                             "permessage-deflate",
                                             lws_extension_callback_pm_deflate,
                                             "permessage-deflate"
                                            },
                                            { NULL, NULL, NULL /* terminator */ }
};

static const struct lws_http_mount mount = {
                                            NULL,	/* linked-list pointer to next*/
                                            "/",		/* mountpoint in URL namespace on this vhost */
                                            resource_path, /* where to go on the filesystem for that */
                                            "test.html",	/* default filename if none given */
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            0,
                                            LWSMPRO_FILE,	/* mount type is a directory in a filesystem */
                                            1,		/* strlen("/"), ie length of the mountpoint */
                                            NULL,

                                            { NULL, NULL } // sentinel
};

static const struct lws_protocol_vhost_options pvo_options = {
                                                              NULL,
                                                              NULL,
                                                              "options",		/* pvo name */
                                                              (char*)&test_options	/* pvo value */
};

static const struct lws_protocol_vhost_options pvo = {
                                                      NULL,				/* "next" pvo linked-list */
                                                      &pvo_options,		/* "child" pvo linked-list */
                                                      "dumb-increment-protocol",	/* protocol name we belong to on this vhost */
                                                      ""				/* ignored */
};

static struct option options[] = {
                                  { "help",	no_argument,		NULL, 'h' },
                                  { "debug",	required_argument,	NULL, 'd' },
                                  { "port",	required_argument,	NULL, 'p' },
                                  { "interface",	required_argument,	NULL, 'i' },
                                  { "closetest",	no_argument,		NULL, 'c' },
                                  { "unix-socket",  required_argument,	NULL, 'U' },
                                  { "pingpong-secs", required_argument,	NULL, 'P' },
                                  { NULL, 0, 0, 0 }
};

int main(int argc, char **argv)
{
  struct lws_context_creation_info info;
  struct lws_vhost *vhost;
  char interface_name[128] = "";
  const char *iface = NULL;
  char cert_path[1024] = "";
  char key_path[1024] = "";
  char ca_path[1024] = "";
  int uid = -1, gid = -1;
  int pp_secs = 0;
  int opts = 0;
  int n = 0;

  /*
   * take care to zero down the info struct, he contains random garbaage
   * from the stack otherwise
   */
  memset(&info, 0, sizeof info);
  info.port = 7681;

  while (n >= 0) {
    n = getopt_long(argc, argv, "ci:hsap:d:DC:K:A:R:vu:g:P:kU:n", options, NULL);
    if (n < 0)
      continue;
    switch (n) {
    case 'u':
      uid = atoi(optarg);
      break;
    case 'g':
      gid = atoi(optarg);
      break;
    case 'd':
      debug_level = atoi(optarg);
      break;
    case 'n':
      /* no dumb increment send */
      test_options |= 1;
      break;
    case 'p':
      info.port = atoi(optarg);
      break;
    case 'i':
      lws_strncpy(interface_name, optarg, sizeof interface_name);
      iface = interface_name;
      break;
    case 'U':
      lws_strncpy(interface_name, optarg, sizeof interface_name);
      iface = interface_name;
      opts |= LWS_SERVER_OPTION_UNIX_SOCK;
      break;
    case 'k':
      info.bind_iface = 1;
      break;
    case 'c':
      close_testing = 1;
      fprintf(stderr, " Close testing mode -- closes on "
              "client after 50 dumb increments"
              "and suppresses lws_mirror spam\n");
      break;
    case 'C':
      lws_strncpy(cert_path, optarg, sizeof(cert_path));
      break;
    case 'K':
      lws_strncpy(key_path, optarg, sizeof(key_path));
      break;
    case 'A':
      lws_strncpy(ca_path, optarg, sizeof(ca_path));
      break;
    case 'P':
      pp_secs = atoi(optarg);
      lwsl_notice("Setting pingpong interval to %d\n", pp_secs);
      break;
    case 'h':
      fprintf(stderr, "Usage: test-server "
              "[--port=<p>] "
              "[-d <log bitfield>]\n");
      exit(1);
    }
  }


  /* tell the library what debug level to emit and to send it to stderr */
  lws_set_log_level(debug_level, NULL);

  printf("Using resource path \"%s\"\n", resource_path);

  info.iface = iface;
  info.protocols = my_protocols;
  info.ws_ping_pong_interval = pp_secs;

  info.gid = gid;
  info.uid = uid;
  info.options = opts | LWS_SERVER_OPTION_VALIDATE_UTF8 | LWS_SERVER_OPTION_EXPLICIT_VHOSTS;
  info.extensions = exts;
  info.timeout_secs = 5;
  info.mounts = &mount;
  info.ip_limit_ah = 24; /* for testing */
  info.ip_limit_wsi = 105; /* for testing */

  context = lws_create_context(&info);
  if (context == NULL) {
    lwsl_err("libwebsocket init failed\n");
    return -1;
  }

  info.pvo = &pvo;
  vhost = lws_create_vhost(context, &info);
  if (!vhost) {
    lwsl_err("vhost creation failed\n");
    return -1;
  }

  info.port++;
  // fops_plat = *(lws_get_fops(context));
  /* override the active fops */
  // lws_get_fops(context)->open = test_server_fops_open;

  n = 0;
  while (n >= 0 && !force_exit) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    n = lws_service(context, 50);                
  }

  lws_context_destroy(context);
  lwsl_notice("libwebsockets-test-server exited cleanly\n");
  return 0;
}

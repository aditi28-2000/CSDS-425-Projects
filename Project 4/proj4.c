
/* Name: Aditi Mondal
   Case Network ID: axm1849
   Filename: proj4.c
   Date created: 26th October, 2023
   Brief Description: This C program monitors a network trace file and processes & analyze each packet in the file to print some aspect of the packet.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/ethernet.h>
// #include <pcap.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

// #define REQUIRED_ARGC 4
#define ERROR 1
#define QLEN 1
#define PROTOCOL "tcp"
#define BUFLEN 1024
#define MAX_PKT_SIZE 1600
#define UDP_HEADER_LENGTH 8
#define SCALING_FACTOR 4 // convert 32-bit words to bytes
#define MEMORY_ALLOC_FAILURE_ERR "Cannot allocate Memory"
#define REALLOC_SIZE 2
#define MAX_TRAFFIC_ARRAY_SIZE 1000

unsigned short cmd_line_flags = 0;
char *trace_file = NULL;
bool summary = false;
bool lengthAnalysis = false;
bool packetPrinting = false;
bool packetCounting = false;
bool unknownArg = false;

/* meta information, using same layout as trace file */
struct meta_info
{
  unsigned int usecs;
  unsigned int secs;
  unsigned short ignored;
  unsigned short caplen;
};

/* record of information about the current packet */
struct pkt_info
{
  unsigned short caplen; /* from meta info */
  double now;            /* from meta info */
  unsigned char pkt[MAX_PKT_SIZE];
  struct ether_header *ethh; /* ptr to ethernet header, if present,
                                otherwise NULL */
  struct ip *iph;            /* ptr to IP header, if present,
                                otherwise NULL */
  struct tcphdr *tcph;       /* ptr to TCP header, if present,
                                otherwise NULL */
  struct udphdr *udph;       /* ptr to UDP header, if present,
                                otherwise NULL */
};

struct pkt_info pinfo;

// creating a structure to store the network traffic information for the Packet Counting mode
struct networkTraffic_info
{
  char src_ip[INET_ADDRSTRLEN];
  char dst_ip[INET_ADDRSTRLEN];
  unsigned long total_pkts;
  unsigned long traffic_volume;
};

struct networkTraffic_info *traffic_info_array;

int traffic_info_index = 0;
size_t traffic_array_size = MAX_TRAFFIC_ARRAY_SIZE;

int usage(char *progname)
{
  fprintf(stderr, "usage: %s -r trace_file -s|-l|-p|-c\n", progname);
  exit(ERROR);
}

void errexit(char *msg)
{
  fprintf(stdout, "%s\n", msg);
  exit(1);
}

// to parse the command line arguments
void parseargs(int argc, char *argv[])
{
  int opt, modeCount = 0, r_option = 0;
  while ((opt = getopt(argc, argv, "r:slpc")) != -1)
  {
    switch (opt)
    {
    case 'r':
      cmd_line_flags = 1;
      r_option = 1;
      if (optarg != NULL && optarg[0] != '-')
        trace_file = optarg;
      else
      {
        fprintf(stderr, "error: no argument for -r specified\n");
        usage(argv[0]);
      }
      break;
    case 's':
      cmd_line_flags = 1;
      summary = true;
      modeCount++;
      break;
    case 'l':
      cmd_line_flags = 1;
      lengthAnalysis = true;
      modeCount++;
      break;
    case 'p':
      cmd_line_flags = 1;
      packetPrinting = true;
      modeCount++;
      break;
    case 'c':
      cmd_line_flags = 1;
      packetCounting = true;
      modeCount++;
      break;
    case '?':
      cmd_line_flags = 1;
      unknownArg = true;
      break;
    default:
      usage(argv[0]);
    }
  }
  if (cmd_line_flags == 0)
  {
    fprintf(stderr, "error: no command line option given\n");
    usage(argv[0]);
  }
  else if (r_option == 0)
  {
    // throw error if mode r is not present, mode r must always be present
    fprintf(stderr, "error: -r not specified\n");
    usage(argv[0]);
  }
  else if (unknownArg)
  {
    // throw error for unknown command line arguments
    fprintf(stderr, "error: Unknown command line argument/ mode specified\n");
    usage(argv[0]);
  }
  else if (modeCount != 1 && modeCount > 1)
  {
    // throw error for multiple modes
    fprintf(stderr, "error: Multiple modes specified\n");
    usage(argv[0]);
  }
  else if (modeCount == 0)
  {
    // throw error if no mode specified
    fprintf(stderr, "error: No mode specified\n");
    usage(argv[0]);
  }
}

unsigned short next_packet(int fd, struct pkt_info *pinfo)
{
  struct meta_info meta;
  int bytes_read;

  memset(pinfo, 0x0, sizeof(struct pkt_info));
  memset(&meta, 0x0, sizeof(struct meta_info));

  /* read the meta information */
  bytes_read = read(fd, &meta, sizeof(meta));
  if (bytes_read == 0)
    return (0);
  if (bytes_read < sizeof(meta))
    errexit("cannot read meta information");
  pinfo->caplen = ntohs(meta.caplen);
  /* TODO: set pinfo->now based on meta.secs & meta.usecs */
  pinfo->now = (double)ntohl(meta.secs) + ((double)ntohl(meta.usecs) / 1000000.0);
  //  printf("pinfo->now: %lf\n", pinfo->now);
  if (pinfo->caplen == 0)
    return (1);
  if (pinfo->caplen > MAX_PKT_SIZE)
    errexit("packet too big");
  /* read the packet contents */
  bytes_read = read(fd, pinfo->pkt, pinfo->caplen);
  if (bytes_read < 0)
    errexit("error reading packet");
  if (bytes_read < pinfo->caplen)
    errexit("unexpected end of file encountered");
  if (bytes_read < sizeof(struct ether_header))
    return (1);
  pinfo->ethh = (struct ether_header *)pinfo->pkt;
  pinfo->ethh->ether_type = ntohs(pinfo->ethh->ether_type);
  if (pinfo->ethh->ether_type != ETHERTYPE_IP)
    /* nothing more to do with non-IP packets */
    return (1);
  if (pinfo->caplen == sizeof(struct ether_header))
    /* we don't have anything beyond the ethernet header to process */
    return (1);
  pinfo->iph = (struct ip *)(pinfo->pkt + sizeof(struct ether_header));
  if (pinfo->iph->ip_p == IPPROTO_TCP)
  {
    /* set pinfo->tcph to the start of the TCP header
       setup values in pinfo->tcph as needed*/
    pinfo->tcph = (struct tcphdr *)(pinfo->pkt + sizeof(struct ether_header) + pinfo->iph->ip_hl * SCALING_FACTOR);
  }
  else if (pinfo->iph->ip_p == IPPROTO_UDP)
  {
    /* set pinfo->udph to the start of the UDP header,
       setup values in pinfo->udph, as needed */
    pinfo->udph = (struct udphdr *)(pinfo->pkt + sizeof(struct ip));
  }

  return (1);
}

// printing the output for the Summary mode
void summary_mode(int fd)
{
  double first_time = 0.0;
  double last_time = 0.0;
  int count_ip_pkts = 0;
  int i = 0;
  while (next_packet(fd, &pinfo) == 1)
  {
    if (i == 0)
    {
      printf("time: first: %lf ", pinfo.now);
      first_time = pinfo.now;
    }
    // Counting the number of IP packets
    if (pinfo.caplen != 0 && (pinfo.ethh != NULL) && (pinfo.ethh->ether_type == ETHERTYPE_IP))
      count_ip_pkts++;
    last_time = pinfo.now;
    i++;
  }
  printf("last: %lf duration: %lf\n", last_time, (last_time - first_time));
  printf("pkts: total: %d ip: %d\n", i, count_ip_pkts);
}

// printing the output for the lengthAnalysis_mode
void lengthAnalysis_mode(int fd)
{
  int payload_len, iphl, trans_hl, iplen;
  while (next_packet(fd, &pinfo) == 1)
  {
    // checking only for the IP packets
    if (pinfo.caplen != 0 && (pinfo.ethh != NULL) && (pinfo.ethh->ether_type == ETHERTYPE_IP))
    {
      printf("%lf %hu ", pinfo.now, pinfo.caplen);
      if (pinfo.iph != NULL)
      {
        iphl = pinfo.iph->ip_hl * SCALING_FACTOR;
        iplen = ntohs(pinfo.iph->ip_len);
        printf("%d %d ", iplen, iphl);
        if (pinfo.tcph != NULL)
        {
          printf("%c ", 'T');
          trans_hl = (pinfo.tcph->th_off) * SCALING_FACTOR;
          payload_len = iplen - iphl - trans_hl;
          if (trans_hl == 0)
            printf("%c %c", '-', '-');
          else
            printf("%d %d", trans_hl, payload_len);
        }
        else if (pinfo.udph != NULL)
        {
          trans_hl = UDP_HEADER_LENGTH;
          payload_len = iplen - iphl - trans_hl;
          printf("%c %d %d", 'U', trans_hl, payload_len);
        }
        else
          printf("%c %c %c", '?', '?', '?');
      }
      else
        printf("- - - - -");
      printf("\n");
    }
  }
}

// updating the total number of packets and traffic volume for each pair of hosts
void updateTraffic_info(char *src_ip, char *dst_ip, unsigned long payload_len)
{
  for (int i = 0; i < traffic_info_index; ++i)
  {
    // checking if the next packet also has the same source ip and destination ip
    if (strcmp(traffic_info_array[i].src_ip, src_ip) == 0 &&
        strcmp(traffic_info_array[i].dst_ip, dst_ip) == 0)
    {
      // updating the number of packets sent and traffic volume for that pair of hosts
      traffic_info_array[i].total_pkts++;
      traffic_info_array[i].traffic_volume += payload_len;
      return;
    }
  }
  // if the number of packets in the trace file exceeds the maximum traffic_array_size reallocate the size of the array
  if (traffic_info_index >= traffic_array_size)
  {
    traffic_array_size *= REALLOC_SIZE;
    traffic_info_array = realloc(traffic_info_array, traffic_array_size * sizeof(struct networkTraffic_info));

    if (traffic_info_array == NULL)
    {
      errexit(MEMORY_ALLOC_FAILURE_ERR);
    }
  }

  // Adding entry for new pair of hosts
  strcpy(traffic_info_array[traffic_info_index].src_ip, src_ip);
  strcpy(traffic_info_array[traffic_info_index].dst_ip, dst_ip);
  traffic_info_array[traffic_info_index].total_pkts = 1;
  traffic_info_array[traffic_info_index].traffic_volume = payload_len;

  traffic_info_index++;
}

int main(int argc, char *argv[])
{
  parseargs(argc, argv);
  char src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
  unsigned int ip_id, ip_ttl, src_port, dst_port, window;
  unsigned long ackno, payload_len;
  int fd = open(trace_file, O_RDONLY);

  if (fd < 0)
  {
    // check return value for file descriptor
    errexit("Cannot open file");
  }
  // if -s mode specified
  if (summary)
  {
    summary_mode(fd);
  }
  // if -l mode specified
  if (lengthAnalysis)
  {
    lengthAnalysis_mode(fd);
  }
  // if -p or -c mode specified
  if (packetPrinting || packetCounting)
  {
    traffic_info_array = malloc(traffic_array_size * sizeof(struct networkTraffic_info));

    if (traffic_info_array == NULL)
    {
      errexit(MEMORY_ALLOC_FAILURE_ERR);
    }

    while (next_packet(fd, &pinfo) == 1)
    {
      // printf("Entering packetPrinting block\n");
      if (pinfo.tcph != NULL && pinfo.tcph->th_off != 0)
      {
        // string representations of IP addresses, in dotted-quad notation
        strcpy(src_ip, inet_ntoa(pinfo.iph->ip_src));
        strcpy(dst_ip, inet_ntoa(pinfo.iph->ip_dst));
        if (packetCounting)
        {
          payload_len = ntohs(pinfo.iph->ip_len) - pinfo.iph->ip_hl * 4 - pinfo.tcph->th_off * SCALING_FACTOR;
          updateTraffic_info(src_ip, dst_ip, payload_len);
        }
        else if (packetPrinting)
        {
          printf("%lf %s ", pinfo.now, src_ip);
          // dst_ip=inet_ntoa(pinfo.iph->ip_dst);
          ip_id = ntohs(pinfo.iph->ip_id);
          ip_ttl = pinfo.iph->ip_ttl;
          src_port = ntohs(pinfo.tcph->th_sport);
          dst_port = ntohs(pinfo.tcph->th_dport);
          window = ntohs(pinfo.tcph->th_win);
          printf("%u %s %u %u %u %u ", src_port, dst_ip, dst_port, ip_id, ip_ttl, window);

          if (pinfo.tcph->th_flags & TH_ACK)
          {
            ackno = ntohl(pinfo.tcph->th_ack);
            printf("%lu\n", ackno);
          }
          else
            printf("-\n");
        }
      }
    }
    if (packetCounting)
    {
      // print packet counting/network traffic information for each pair of hosts in the trace file
      for (int i = 0; i < traffic_info_index; ++i)
      {
        printf("%s %s %lu %lu\n",
               traffic_info_array[i].src_ip,
               traffic_info_array[i].dst_ip,
               traffic_info_array[i].total_pkts,
               traffic_info_array[i].traffic_volume);
      }
    }
  }
  close(fd);
  free(traffic_info_array);
  exit(0);
}

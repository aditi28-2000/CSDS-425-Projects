# Name: Aditi Mondal
# Case Network ID: axm1849

import pandas as pd
import matplotlib.pyplot as plt
# Read the CSV file into a Pandas DataFrame
df = pd.read_csv('/Users/aditimondal/Desktop/TCPfileanalysis.csv',encoding='latin1')

protocol_counts = df.groupby(['Destination Port', 'Protocol']).size().reset_index(name='Count')

# Filter the DataFrame for each protocol
tls_v1_2_data = df[df['Protocol'] == 'TLSv1.2']
tls_v1_3_data = df[df['Protocol'] == 'TLSv1.3']

# Group each DataFrame by Source Port
tls_v1_2_source_ports = tls_v1_2_data['Source Port'].value_counts()
tls_v1_3_source_ports = tls_v1_3_data['Source Port'].value_counts()

# Display the top source ports for each protocol

print("\nTop Source Ports used for TLSv1.2:")
tls_v1_2_source_ports = tls_v1_2_source_ports.reset_index().rename(columns={'index': 'Source Port', 'Source Port': 'Packets'})
print(tls_v1_2_source_ports.head(3))
print("\nTop Source Ports used for TLSv1.3:")
tls_v1_3_source_ports = tls_v1_3_source_ports.reset_index().rename(columns={'index': 'Source Port', 'Source Port': 'Packets'})
print(tls_v1_3_source_ports.head(3))

protocol_distribution = df['Protocol'].value_counts()

# Plot the distribution
plt.figure(figsize=(10, 6))
ax = protocol_distribution.plot(kind='bar', color='skyblue')

# Annotate each bar with its count
for i, count in enumerate(protocol_distribution):
    ax.text(i, count + 0.1, str(count), ha='center', va='bottom')

plt.title('Protocol Distribution')
plt.xlabel('Protocol')
plt.ylabel('Number of Packets')
plt.show()

### RTT for the TCO Connection establishment ### 
# Read CSV file into a Pandas DataFrame
df2 = pd.read_csv('/Users/aditimondal/Desktop/TCP_RTT.csv',encoding='latin1')

num_rows = df2.shape[0]

print(f"Number of TCP packets involved in the intial TCP connection establishment: {num_rows}")

plt.figure(figsize=(10, 6))
plt.plot(df2['RTT'], marker='o', linestyle='-', color='b')
plt.title('RTT Over Time')
plt.xlabel('Packet Index')
plt.ylabel('RTT (seconds)')
plt.grid(True)
plt.show()

# Print RTT statistics
rtt_statistics = df2['RTT'].describe()
print("\nRTT Statistics:")
print(rtt_statistics)

### Calculate Window Size Distribution ###
plt.figure(figsize=(10, 6))
plt.plot(df['Time'], df['Calculated window size'], label='Calculated Window Size')
plt.xlabel('Time(seconds)')
plt.ylabel('Calculated Window Size')
plt.title('Calculated Window Size Over Time')
plt.legend()
plt.show()
w_statistics = df['Calculated window size'].describe()
print("\n Statistics:")
print(w_statistics)

### TCP connection reset ###
TCP_reset = df[df['Info'].str.contains('RST')]
TCP_reset_count = len(TCP_reset)
print(f"Total TCP resets identified in the network : {TCP_reset_count}")
TCP_reset_maxports = df[df['Info'].str.contains('RST') & ((df['Source Port'] == 80) | (df['Destination Port'] == 80) | (df['Source Port'] == 443) | (df['Destination Port'] == 443))]
TCP_reset_count1 = len(TCP_reset_maxports)

print(f"Count of TCP reset connections with Source or Destination Port 80 or 443: {TCP_reset_count1}")


# Count TCP reset packets with source or destination port 80
reset_port_80 = len(df[(df['Info'].str.contains('RST')) & ((df['Source Port'] == 80) | (df['Destination Port'] == 80))])

# Count TCP reset packets with source or destination port 443
reset_port_443 = len(df[(df['Info'].str.contains('RST')) & ((df['Source Port'] == 443) | (df['Destination Port'] == 443))])

# Plotting the bar graph
plt.figure(figsize=(10, 6))
ports = ['Port 80', 'Port 443']
counts = [reset_port_80, reset_port_443]

bars = plt.bar(ports, counts, color=['blue', 'green'])
plt.xlabel('Source/Destination Port')
plt.ylabel('Number of TCP Reset Connections')
plt.title('TCP Reset Packets Count with Source/Destination Port 80 and 443')

# Adding counts as annotations
for bar, count in zip(bars, counts):
    plt.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 10, str(count), ha='center', va='bottom')

plt.show()

### TCP DUP ACKS ###
dup_ack_packets = df[df['Info'].str.contains('Dup ACK')]

# Group by time and count the number of DUP ACK packets at each time
dup_ack_count_over_time = dup_ack_packets.groupby('Time').size()

# Plotting the line graph
plt.figure(figsize=(12, 6))
plt.plot(dup_ack_count_over_time.index, dup_ack_count_over_time.values, label='DUP ACK Packets', color='blue')
plt.xlabel('Time(seconds)')
plt.ylabel('Count')
plt.title('DUP ACK Packets Over Time')
plt.legend()
plt.show()

### TCP Retransmissions ###

# Add new columns for spurious retransmission and fast retransmission
df['spurious_retransmission'] = df['Info'].str.contains('Spurious Retransmission', case=False).astype(int)
df['fast_retransmission'] = df['Info'].str.contains('Fast Retransmission', case=False).astype(int)

retransmission_packets = df[df['Info'].str.contains('Retransmission')]
retransmission_count = len(retransmission_packets)
print(f"Count of Packets Retransmitted: {retransmission_count}")
retransmission_df = retransmission_packets.copy()
spurious_retransmission_packets = retransmission_df[retransmission_df['Info'].str.contains('Spurious Retransmission')]
spurious_retransmission_count = len(spurious_retransmission_packets)
print(f"Out of 3206 retransmitted packets the count of packets that were spuriously retransmitted: {spurious_retransmission_count}")

#fast retransmissions
fast_retransmissions = df[df['fast_retransmission'] == 1]
fast_retransmission_count = len(fast_retransmissions)
print(f"Out of 3206-692= 2514 retransmitted packets the count of packets that were part of fast retransmissions: {fast_retransmission_count}")

# Filter rows in the retransmission DataFrame where 'TCP Segment Len' is zero
zero_segment_length_packets = retransmission_df[retransmission_df['TCP Segment Len'] == 0]

# Print the count of packets with TCP segment length equal to zero
zero_segment_length_count = len(zero_segment_length_packets)
print(f"Count of Retransmission Packets with TCP Segment Length equal to zero: {zero_segment_length_count}")
# Filter rows for spurious retransmissions and fast retransmissions
spurious_retransmissions = df[df['spurious_retransmission'] == 1]
fast_retransmissions = df[df['fast_retransmission'] == 1]

# Create line plots for spurious and fast retransmissions over time
plt.figure(figsize=(12, 8))

plt.plot(spurious_retransmissions['Time'], spurious_retransmissions.index, label='Spurious Retransmissions', marker='o', linestyle='-')
plt.plot(fast_retransmissions['Time'], fast_retransmissions.index, label='Fast Retransmissions', marker='o', linestyle='-')

plt.title('Distribution of Spurious and Fast Retransmissions Over Time')
plt.xlabel('Time(seconds)')
plt.ylabel('Packet Index')
plt.legend()
plt.grid(True)
plt.show()


### TCP Window Update ###
window_update_packets = df[df['Info'].str.contains('Window Update')]
window_updates_over_time = window_update_packets.groupby('Time').size()
plt.figure(figsize=(12, 6))
plt.plot(window_updates_over_time.index, window_updates_over_time.values, label='Window Update Packets', color='blue', marker='o')
plt.xlabel('Time (seconds)')
plt.ylabel('Packet Count')
plt.title('TCP Window Updates Over Time')
plt.legend()
plt.grid(True)
plt.show()





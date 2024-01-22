import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

#TCP packet printing mode Output
column_titles=["ts", "src_ip", "src_port", "dst_ip", "dst_port", "ip_id", "ip_ttl", "window", "ackno"] 
packetPrinting_output = pd.read_csv("/Users/aditimondal/Desktop/packetPrinting_mode.out",delim_whitespace=True, header=None, names=column_titles)

#checking the top 10 most accessed destination ports in the trace file
ports = packetPrinting_output['dst_port'].value_counts()[:10]
df=pd.DataFrame({'dst_port': ports.index, 'Packets': ports.values})
print(df)

# getting the most accessed destination port
max_port = ports.idxmax()
max_count = ports.max()

# the result
print(f"The most accessed destination port is {max_port} with a total count of {max_count} packets")

# filtering rows where dst_port is 80
filtered_rows = packetPrinting_output[packetPrinting_output['dst_port'] == 80]

#most frequent window size for dst_port 80
windowsizes=(filtered_rows['window'].value_counts())
most_frequent_windowsize = windowsizes.idxmax()
print(f"The most frequent window size adapted for destination port 80 is {most_frequent_windowsize}")

# checking the frequency distribution of the TCP window sizes for the destination ports = 80
plt.figure(figsize=(12, 6))
sns.histplot(filtered_rows['window'], kde=True, bins=30, color='skyblue')
plt.title('Frequency distribution of Window Sizes for dst_port=80')
plt.xlabel('Window Size')
plt.ylabel('Frequency')
plt.show()


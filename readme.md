#	`EPHERFS` utility v1.0.0
###	What is `EPHERFS`?
####	It is a simple file system designed to simplify the process of creating an OS
####	More details: [here](./docs/epherfs-v-ktsn.md)
###	What does it do?
####	Allows you to work with or create an `EPHERFS` image
###	How to use it?
>	`-help` - usage

>	`-cft <dir> <out>` - create file system table from directory

>	`-cp-ft <name(31)> <num-of-sectors> <file-table-path> <num-of-files> <out>` - create partition from file table
>>	`<name(31)>` - name of the new partition (max length - 31)
>>	`<num-of-sectors>` - maximum number of sectors in a partition
>>	`<num-of-files>` - current number of files in the file table

>	`-cp-d <name(31)> <num-of-sectors> <dir> <out> - create partition from file dir` - create partition from directory
>>	`<name(31)>` - name of the new partition (max length - 31)
>>	`<num-of-sectors>` - maximum number of sectors in a partition

>	`-c-bl <reserved-data-file> <out> <partitions...>` - create EPHERFS image
>>	`<reserved-data-file>` - the path to the file with the reserved data (there may be a loader, etc.)
>>	`<partitions...>` - path to files with partition

>	`-e <fs-image> <out-dir>` - unpack files & folders from fs image
#!/usr/bin/perl -w
my $file = "test-cgi-img.gif";
## my $length = (stat($file)) [10];
## (stat($file)) [10]; is the inode change time in seconds since  00:00 January 1, 1970 GMT. 
my $length = (stat($file)) [7];
print STDOUT "HTTP/1.1 200 OK \r\n";
print STDOUT "Content-type: image/gif\n";
print STDOUT "\r\n";
print STDOUT "Connection: Keep-Alive\r\n";
binmode STDOUT;
open (FH,'<', $file) || die "Could not open $file: $!";
my $buffer = "";
while (read(FH, $buffer, 10240)) {
    print $buffer;
}
close(FH);
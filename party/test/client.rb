require 'rubygems'
require 'json'
require "socket"

$host = "127.0.0.1"
$port = 10000

client = nil
data = nil

params = Hash.new

class String
  def xor x
    if x.is_a?(String)
      r = ''
      j = 0
      0.upto(self.size-1) do |i|
        r << (self[i].ord^x[j].ord).chr
        j+=1
        j=0 if j>= x.size
      end
      r
    else
      r = ''
      0.upto(self.size-1) do |i|
        r << (self[i].ord^x).chr
      end
      r
    end
  end
end

# Assumes 'msg' is single-byte encoded
# and not larger than 4,3 GB ((2**(4*8)-1) bytes)
def send_msg(handle, msg)
	handle.write([msg.length].pack('I') + msg)
end

def recv_msg(handle)
	if message_size = handle.read(4) # sizeof (I)
		message_size = message_size.unpack('I')[0]
		handle.read(message_size)
	end
end
	
begin
timeout = 5
client = TCPSocket.new($host, $port)

secs = Integer(timeout)
usecs = Integer((timeout - secs) * 1_000_000)
optval = [secs, usecs].pack("l_2")
client.setsockopt Socket::SOL_SOCKET, Socket::SO_RCVTIMEO, optval
client.setsockopt Socket::SOL_SOCKET, Socket::SO_SNDTIMEO, optval

params['cmd'] = 1001
params['uid'] = 3
params['skey'] = "abc"
data = params.to_json
data = data.xor(13)
send_msg(client, data)
str = recv_msg(client)
puts str.xor(13)
str = recv_msg(client)
puts str.xor(13)
rescue => err
	puts err
ensure
	if client != nil
		client.close
	end
end


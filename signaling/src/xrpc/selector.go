package xrpc

import(
	"errors"
	"net"
	"sync"
)

type ServerSelector interface {
	PickServer() (net.Addr, error)
}

type RoundRobinSelector struct {
	sync.RWMutex
	addrs []net.Addr
	curIndex int
}

func (rrs *RoundRobinSelector) SetServer(servers []string) error {
	if len(servers) == 0 {
		return errors.New("no servers provided")
	}

	addrs := make([]net.Addr, len(servers))
	for i,server := range servers{
		tcpAddr,err := net.ResolveTCPAddr("tcp", server)
		if err != nil{
			return err
		}

		addrs[i] = tcpAddr
	}

	rrs.Lock()
	rrs.addrs = addrs
	rrs.Unlock()

	return nil
}

// 轮询实现负载均衡
func (rrs *RoundRobinSelector) PickServer() (net.Addr, error) {
	rrs.Lock()
	index := rrs.curIndex
	rrs.curIndex++
	if rrs.curIndex >= len(rrs.addrs) {
		rrs.curIndex = 0
	}
	rrs.Unlock()

	rrs.RLock()
	defer rrs.RUnlock()

	if len(rrs.addrs) == 0 {
		return nil,errors.New("no available servers")
	}

	return rrs.addrs[index], nil
}
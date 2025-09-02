package framework

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"strconv"
	"strings"
	"time"

	"signaling/src/framework/xrpc"
)

var xrpcClients map[string]*xrpc.Client = make(map[string]*xrpc.Client)

func loadXrpc() error {
	sections := configFile.GetSectionList()

	for _, section := range sections {
		if !strings.HasPrefix(section, "xrpc.") {
			continue
		}

		mSection, err := configFile.GetSection(section)
		if err != nil {
			return err
		}

		// server
		values, ok := mSection["server"]
		if !ok {
			return errors.New("no server field in config file")
		}

		arrServer := strings.Split(values, ",")
		for i, server := range arrServer {
			arrServer[i] = strings.TrimSpace(server)
		}

		client := xrpc.NewClient(arrServer)

		if values, ok := mSection["connectTimeout"]; ok {
			if connectTimeout, err := strconv.Atoi(values); err == nil {
				client.ConnectTimeout = time.Duration(connectTimeout) * time.Millisecond
			}
		}

		if values, ok := mSection["readTimeout"]; ok {
			if readTimeout, err := strconv.Atoi(values); err == nil {
				client.ReadTimeout = time.Duration(readTimeout) * time.Millisecond
			}
		}

		if values, ok := mSection["writeTimeout"]; ok {
			if writeTimeout, err := strconv.Atoi(values); err == nil {
				client.WriteTimeout = time.Duration(writeTimeout) * time.Millisecond
			}
		}

		xrpcClients[section] = client
	}

	return nil
}

func Call(serviceName string, request interface{}, response interface{},
	logId uint32) error {
	fmt.Println("call " + serviceName)

	client, ok := xrpcClients["xrpc."+serviceName]
	if !ok {
		return fmt.Errorf("[%s] service not found", serviceName)
	}

	content, err := json.Marshal(request)
	if err != nil {
		return err
	}

	req := xrpc.NewRequest(bytes.NewReader(content), logId)
	resp, err := client.Do(req)
	if err != nil {
		return err
	}

	err = json.Unmarshal(resp.Body, response)
	if err != nil {
		return err
	}

	return nil
}

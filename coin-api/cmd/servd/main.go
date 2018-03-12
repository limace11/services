package servd

import (
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"sync"

	"github.com/labstack/echo"
	"github.com/labstack/echo/middleware"
)

type Status struct {
	sync.Mutex
	Stats map[string]interface{}
}

// Start starts the server
func Start(config *Config) (*echo.Echo, error) {
	e := echo.New()
	e.Use(middleware.GzipWithConfig(middleware.DefaultGzipConfig))
	e.Use(middleware.RecoverWithConfig(middleware.DefaultRecoverConfig))

	// e.Pre(middleware.MethodOverride())
	hMulti := newHandlerMulti()

	var cert []byte

	if config.Bitcoin.TLS {
		f, err := os.Open(config.Bitcoin.CertFile)

		if err != nil {
			log.Fatal(err)
		}

		cert, err = ioutil.ReadAll(f)

		if err != nil {
			log.Fatal(err)
		}
	}

	hBTC, err := newHandlerBTC(config.Bitcoin.NodeAddress,
		config.Bitcoin.User,
		config.Bitcoin.Password,
		!config.Bitcoin.TLS,
		cert,
		config.Bitcoin.BlockExplorer)

	apiGroupV1 := e.Group("/api/v1")
	skyGroup := apiGroupV1.Group("/sky")
	btcGroup := apiGroupV1.Group("/btc")

	// ping server
	// apiGroupV1.GET("/ping", hMulti.generateSeed)
	// show currencies and api's list
	// apiGroupV1.GET("/list", hMulti.generateSeed)
	// generate keys
	skyGroup.POST("/keys", hMulti.generateKeys)
	// generate address
	skyGroup.POST("/address/:key", hMulti.generateSeed)
	// check the balance (and get unspent outputs) for an address
	skyGroup.GET("/address/:address", hMulti.checkBalance)
	// sign a transaction
	skyGroup.POST("/transaction/sign/:sign", hMulti.signTransaction)
	// inject transaction into network
	skyGroup.PUT("/transaction/:netid/:transid", hMulti.injectTransaction)
	// check the status of a transaction (tracks transactions by transaction hash)
	skyGroup.GET("/transaction/:transid", hMulti.checkTransaction)
	// Generate key pair
	btcGroup.POST("/keys", hBTC.generateKeyPair)
	// // BTC generate address based on public key
	btcGroup.POST("/address", hBTC.generateAddress)
	// BTC check the balance (and get unspent outputs) for an address
	btcGroup.GET("/address/:address", hBTC.checkBalance)
	// BTC check the status of a transaction (tracks transactions by transaction hash)
	btcGroup.GET("/transaction/:transid", hBTC.checkTransaction)

	statusFunc := func(ctx echo.Context) error {
		status := Status{
			Stats: make(map[string]interface{}),
		}
		// Collect statuses from handlers
		hMulti.CollectStatus(&status)
		hBTC.CollectStatuses(&status)
		ctx.JSON(http.StatusOK, status)

		return nil
	}

	// Just for basic service health checking
	e.GET("/health", func(ctx echo.Context) error {
		ctx.NoContent(http.StatusOK)
		return nil
	})

	e.GET("/status", statusFunc)
	// e.StartAutoTLS()
	err = e.Start(config.Server.ListenStr)
	e.Logger.Fatal(err)
	return e, err
}
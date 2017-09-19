NetExec
=======

# Building 

        cmake .
        make
        
# Request examples

## Get job
        curl -s -k -X POST -H 'Content-type: application/json' -H 'Auth-token: 1234' -d'{"id": "44", "jsonrpc": "2.0", "method": "get_job", "params": {"id": "id_xxx61"}}' https://localhost:3333  | jq .

## Run job
        curl -s -k -X POST -H 'Content-type: application/json' -H 'Auth-token: 1234' -d'{"jsonrpc": "2.0", "method": "run_job", "params": {"id": "xxx17", "name": "run-ls", "cmd": ["/bin/ls", "-l", "/usr"]}, "id": 44}' https://localhost:3333 | jq .




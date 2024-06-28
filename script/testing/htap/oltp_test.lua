function prepare()
   local drv = sysbench.sql.driver()
   local con = drv:connect()

   con:query("CREATE TABLE sbtest1 (id INTEGER PRIMARY KEY, k INTEGER NOT NULL DEFAULT 0)")
   con:query("CREATE TABLE sbtest2 (id INTEGER PRIMARY KEY, k INTEGER NOT NULL DEFAULT 0)")
   con:query("CREATE TABLE sbtest3 (id INTEGER PRIMARY KEY, k INTEGER NOT NULL DEFAULT 0)")
   con:query("CREATE TABLE sbtest4 (id INTEGER PRIMARY KEY, k INTEGER NOT NULL DEFAULT 0)")
   con:query("CREATE TABLE sbtest5 (id INTEGER PRIMARY KEY, k INTEGER NOT NULL DEFAULT 0)")
   con:query("CREATE TABLE sbtest6 (id INTEGER PRIMARY KEY, k INTEGER NOT NULL DEFAULT 0)")

   for i = 1, 100 do
      con:query(string.format("INSERT INTO sbtest1 (id, k) VALUES (%d, %d)", i, i))
      con:query(string.format("INSERT INTO sbtest2 (id, k) VALUES (%d, %d)", i, i))
      con:query(string.format("INSERT INTO sbtest3 (id, k) VALUES (%d, %d)", i, i))
      con:query(string.format("INSERT INTO sbtest4 (id, k) VALUES (%d, %d)", i, i))
      con:query(string.format("INSERT INTO sbtest5 (id, k) VALUES (%d, %d)", i, i))
      con:query(string.format("INSERT INTO sbtest6 (id, k) VALUES (%d, %d)", i, i))
   end
end

function cleanup()
   local drv = sysbench.sql.driver()
   local con = drv:connect()

   con:query("DROP TABLE IF EXISTS sbtest1")
   con:query("DROP TABLE IF EXISTS sbtest2")
   con:query("DROP TABLE IF EXISTS sbtest3")
   con:query("DROP TABLE IF EXISTS sbtest4")
   con:query("DROP TABLE IF EXISTS sbtest5")
   con:query("DROP TABLE IF EXISTS sbtest6")
end

function thread_init()
   drv = sysbench.sql.driver()
   con = drv:connect()
end

function event()
   con:query(string.format("UPDATE sbtest1 SET k = %d WHERE id = %d", sysbench.rand.uniform(1, 100), sysbench.rand.uniform(1, 100)))
   con:query(string.format("UPDATE sbtest2 SET k = %d WHERE id = %d", sysbench.rand.uniform(1, 100), sysbench.rand.uniform(1, 100)))
   con:query(string.format("UPDATE sbtest3 SET k = %d WHERE id = %d", sysbench.rand.uniform(1, 100), sysbench.rand.uniform(1, 100)))
   con:query(string.format("UPDATE sbtest4 SET k = %d WHERE id = %d", sysbench.rand.uniform(1, 100), sysbench.rand.uniform(1, 100)))
   con:query(string.format("UPDATE sbtest5 SET k = %d WHERE id = %d", sysbench.rand.uniform(1, 100), sysbench.rand.uniform(1, 100)))
   con:query(string.format("UPDATE sbtest6 SET k = %d WHERE id = %d", sysbench.rand.uniform(1, 100), sysbench.rand.uniform(1, 100)))
end

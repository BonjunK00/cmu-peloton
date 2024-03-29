SELECT SUM(sbtest1.k + sbtest2.k + sbtest3.k + sbtest4.k + sbtest5.k + sbtest6.k)
FROM sbtest1, sbtest2, sbtest3, sbtest4, sbtest5, sbtest6
WHERE sbtest1.id = sbtest2.id AND sbtest2.id = sbtest3.id AND sbtest3.id = sbtest4.id AND sbtest4.id = sbtest5.id AND sbtest5.id = sbtest6.id;

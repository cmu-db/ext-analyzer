# Initialization tests -- most units will start including this.

test "(init) Restart killed instances" {
    foreach type {redis} {
        foreach_${type}_id id {
            if {[get_instance_attrib $type $id pid] == -1} {
                puts -nonewline "$type/$id "
                flush stdout
                restart_instance $type $id
            }
        }
    }
}

test "Cluster nodes are reachable" {
    foreach_redis_id id {
        # Every node should be reachable.
        wait_for_condition 1000 50 {
            ([catch {R $id ping} ping_reply] == 0) &&
            ($ping_reply eq {PONG})
        } else {
            catch {R $id ping} err
            fail "Node #$id keeps replying '$err' to PING."
        }
    }
}

test "Cluster nodes hard reset" {
    foreach_redis_id id {
        R $id cluster reset hard
#        R $id cluster leaving no
        R $id config set cluster-node-timeout 1000
        R $id config rewrite
    }
}

test "Cluster Join and auto-discovery test" {
    # Join node 0 with 1, 1 with 2, ... and so forth.
    # If auto-discovery works all nodes will know every other node
    # eventually.
    set ids {}
    foreach_redis_id id {lappend ids $id}
    for {set j 0} {$j < [expr [llength $ids]-1]} {incr j} {
        set a [lindex $ids $j]
        set b [lindex $ids [expr $j+1]]
        set b_port [get_instance_attrib redis $b port]
        R $a cluster meet 127.0.0.1 $b_port
    }

    foreach_redis_id id {
        wait_for_condition 1000 50 {
            [R $id DISQUE INFO cluster.nodes.reachable]+1 == [llength $ids]
        } else {
            fail "Cluster failed to join into a full mesh."
        }
    }
}

#�޸�2016-34-14 ���ӷ������˵����ɾ��target_sys��usr.img���֣����Ӵ���������˵��
#�޸�2016-3-23  ��ʼ�汾
#author��henrisk

һ���������壺
    boot���������ں˾���
    rootfs����������ϵͳ���������/lib, /bin, /etc, /sbin�ȷ�/usr��·����wifi֧�ֻ�����alsa֧�ֻ�����OTA������
    extend��������չϵͳ���������/usr Ӧ�÷�����
    ������������Ϊ��������

    private�������洢SN�ŷ���
    misc������ϵͳ״̬��ˢ��״̬����
    UDISK�������û����ݷ�����/mnt/UDISK��
    overlayfs�������洢overlayfs��������
    �������Ϊ����������

����������Сע�����
    ������С�ڷ���ʹ��sys_partition.fex�ж���
    size���㷨���� 8192/2/1024 = 4M
    
    a������boot������С��boot������С��Ҫ�����ں����ã���ҪС�ڵ���sys_partition.fex�ж����boot��ǩ�Ķ��壺
    �磺 
        [partition]
            name         = boot
            size         = 8192
            downloadfile = "boot.fex"
            user_type    = 0x8000

        boot���������С���趨��
        make menuconfig
            Target Images  --->
                *** Image Options ***
                (4) Boot filesystem partition size (in MB)
    
    b��rootfs�����Ĵ�С������Ҫͨ��make menuconfigȥ�趨��ֱ�Ӹ��ݾ����С�޸ķ����ļ����ɡ�
        1������һЩС����flash�ķ�������16M��������/bin �´�������߼����򡢰汾���Ƴ������ؾ�����򡢲������������Լ������ļ�����Щ�ļ��ڱ���ʱӦ��install��/bin����/lib�£�
       �����ڹ̼�������󣬲鿴bin/sunxi(sun5i)/��rootfs.img�Ĵ�С�ھ���sys_partition.fex��rootfs�����Ĵ�С����
       \*0*/ $ ll bin/sun5i/rootfs.img
        -rw-r--r-- 1 heweihong heweihong  1835008  4�� 14 16:44 bin/sun5i/rootfs.img

        2�����ڴ�����flash�ķ�������128M���ϣ��������㹻��flash�ռ����ؾ��񣩣�����Ҫ1������ЩOTA����ĳ���ֱ�Ӳ鿴rootfs.img�Ĵ�С�趨�����ļ����ɡ�

    c��extend�����Ĵ�С����Ҫ���Ƕ�����棺
        1������� usr.img�Ĵ�С
        2��make_ota_image��initramfs����Ĵ�С
        �磺
        \*0*/ $ ll build_dir/target-arm_cortex-xxxxxxxx/linux-sun5i��linux-sunxi��/
        -rw-r--r--  1 heweihong heweihong   479232  4�� 14 16:44 usr.squashfs
        -rwxr-xr-x  1 heweihong heweihong  5510192  4�� 14 16:44 zImage-initramfs*
        ȡ�������ֵ��������һЩ��������
        
        �������ֵ����Ϊinitramfs����Ĵ�С
        make menuconfig
            Target Images  --->
                *** Image Options ***
                (8) Boot-Recovery initramfs filesystem partition size (in MB)

    d������������private��misc��ʹ��Ĭ�ϵĴ�С����
    e��ʣ�µĿռ�ȫ���Զ��������UDISK����
    
    �ر�ע�⣺��Щ������С����ͨ��OTAȥ�޸ĵģ����Զ��ڴ�����flash�ķ�����Ӧ������������������ƣ�������adc���㣩������������㹻���������������OTA�������ݵ�����
    ����С����flash�ķ�������Ҫ�����������ǵ�����ط����Ĵ�С��

����misc-upgrade����
1. misc-upgrade �ǻ���С����flash�������»��ַ�������misc������extend����Ϊý����Ƶ�OTA����

2. OTA�����SDK����˵����(SDK��Ŀ¼)
   ����������
   source scripts/setenv.sh
   
   �������
   make_ota_image (���°汾�����Ѿ��ɹ��������¼�̼��Ļ����Ļ����ϣ����OTA����)
   make_ota_image --force (���±����°汾���룬Ȼ���ٴ��OTA����)
   
3. OTA�����˵����
    \*0*/ $ ll bin/sunxi��sun5i��/ota/ 
    ?????? 20856
    -rw-rw-r-- 1 heweihong heweihong  5731339  3?? 23 15:48 ramdisk_sys.tar.gz
    -rw-rw-r-- 1 heweihong heweihong 10335244  3?? 23 15:48 target_sys.tar.gz
    -rw-rw-r-- 1 heweihong heweihong  5116895  3?? 23 15:48 usr_sys.tar.gz
    
    ����tar������OTA�ľ����
    ramdisk_sys.tar.gz��ramdisk����Ҫ�����ں˷�����rootfs����ʱʹ�ã���ֹ��д���̵��磬���»�����ש��
    target_sys.tar.gz�� ϵͳ���������ں˷�����rootfs������
    usr_sys.tar.gz��    Ӧ�÷�����������extend������ֻ��Ҫʹ���������
    
4. С����OTA�������
    ��ѡ������-f -p ��ѡһ
    aw_upgrade_process.sh -f ��������ϵͳ���ں˷�����rootfs������extend������ʹ��ramdisk_sys.tar.gz target_sys.tar.gz usr_sys.tar.gz��
    aw_upgrade_process.sh -p ����Ӧ�÷�����extend������ʹ��usr_sys.tar.gz��
    
    ��ѡ����: -l
    aw_upgrade_process.sh -p(-f) -l /mnt/UDISK/misc-upgrade
    
    a�����ڴ�����flash��������ʹ�ñ��ؾ��������������غ����������ramdisk��target��usr��������/mnt/UDISK/misc-upgrade�У������ϵ����
    �����Զ���д�����������ڼ���磬��������������Ҳ���Զ������д������Ҫ�������硣
    b������С����flas��������ʹ��-l����������������������󣬻���Ҫ������ص��������س����ȡ���񣨼���5��˵����
    
5. �ű��ӿ�˵����
    ����С����flash�ķ�������Ҫʵ�����湳�ӽű�
    aw_upgrade_vendor.sh
    
    ʵ�������߼�
    check_network_vendor(){
        return 0 �����ɹ����磺����pingͨOTA�����������
        return 1 ����ʧ��
    }
    
    ��������Ŀ�꾵�� $1��ramdisk_sys.tar.gz $2��/tmp
    download_image_vendor(){
        # $1 image name  $2 DIR  $@ others
        rm -rf $2/$1
        echo "wget $ADDR/$1"
        wget $ADDR/$1 -P $2
    }
    
    ��ʼ��д����״̬��
    aw_upgrade_process.sh -p ��������Ӧ�÷�����ģʽ�£�����0��ʼд����  1��д����
    aw_upgrade_process.sh -f ������������ֵ
    upgrade_start_vendor(){
        # $1 mode: upgrade_pre,boot-recovery,upgrade_post
        #return   0 -> start upgrade;  1 -> no upgrade
        #reutrn value only work in nornal mode
        #nornal mode: $NORMAL_MODE
        echo upgrade_start_vendor $1
        return 0
    }
    
    д�������
    upgrade_finish_vendor(){
        #set version or others
        reboot -f
    }

    -f����˳�� 
            check_network_vendor ->
              upgrade_start_vendor ->             
                download_image_vendor (ramdisk_sys.tar.gz)->
                  �ڲ���д����������߼��������Ѿ�ʹ�þ���ռ���ڴ棩 ->
                    download_image_vendor(target_sys.tar.gz) ->
                      �ڲ���д����������߼��������Ѿ�ʹ�þ���ռ���ڴ棩 ->
                        download_image_vendor(usr_sys.tar.gz) ->
                          �ڲ���д����������߼��������Ѿ�ʹ�þ���ռ���ڴ棩 ->
                            upgrade_finish_vendor
    -p����˳��
            check_network_vendor ->
              download_image_vendor (usr_sys.tar.gz) ->
                upgrade_start_vendor ->             
                  ��ⷵ��ֵ����д ->
                    upgrade_finish_vendor
                    
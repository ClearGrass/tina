1. misc_upgrade �ǻ���С����flash�������»��ַ�������misc������extend����Ϊý����Ƶ�OTA����

2. OTA�����SDK����˵����(SDK��Ŀ¼)
   ����������
   source scripts/setenv.sh
   
   �������
   make_ota_image (���°汾�����Ѿ��ɹ��������¼�̼��Ļ����Ļ����ϣ����OTA����)
   make_ota_image --force (���±����°汾���룬Ȼ���ٴ��OTA����)
   
3. OTA�����˵����
    \*0*/ $ ll bin/sunxi/ota/ 
    ?????? 20856
    drwxrwxr-x 5 heweihong heweihong     4096  3?? 23 15:48 ./
    drwxr-xr-x 5 heweihong heweihong     4096  3?? 23 15:48 ../
    -rw-rw-r-- 1 heweihong heweihong   143172  3?? 23 15:47 .config.old
    drwxrwxr-x 2 heweihong heweihong     4096  3?? 23 15:48 ramdisk_sys/
    -rw-rw-r-- 1 heweihong heweihong  5731339  3?? 23 15:48 ramdisk_sys.tar.gz
    drwxrwxr-x 2 heweihong heweihong     4096  3?? 23 15:47 target_sys/
    -rw-rw-r-- 1 heweihong heweihong 10335244  3?? 23 15:48 target_sys.tar.gz
    drwxrwxr-x 2 heweihong heweihong     4096  3?? 23 15:47 usr_sys/
    -rw-rw-r-- 1 heweihong heweihong  5116895  3?? 23 15:48 usr_sys.tar.gz
    
    ����tar������OTA�ľ����
    ramdisk_sys.tar.gz��ramdisk����Ҫ�����ں˷�����rootfs����ʱʹ�ã�
    target_sys.tar.gz�� ����ϵͳ���������ں˷�����rootfs������extend������usr_sys.tar.gz���������ʹ�õ���
    usr_sys.tar.gz��    Ӧ�÷�����������extend������ֻ��Ҫʹ���������
    
4. С����OTA�������
    aw_upgrade_process.sh --force ��������ϵͳ���ں˷�����rootfs������extend������ʹ��ramdisk_sys.tar.gz target_sys.tar.gz��
    aw_upgrade_process.sh --post  ����Ӧ�÷�����extend������ʹ��usr_sys.tar.gz�� 
    
5. �ű��ӿ�˵����
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
    aw_upgrade_process.sh --force ��--post������������������ģʽ�£�����0��ʼд����  1��д���� 
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
        #set version
        write_misc -v henrisk_test_v1 -s ok
        reboot -f
    }

    ����˳�� check_network_vendor -> download_image_vendor -> download_image_vendor ... -> upgrade_start_vendor -> upgrade_finish_vendor
# Ubuntu/Debian service install

Installing the current module:

```sh
  sudo install -m 644 ddns/ubuntu/ddns-ssh.service /etc/systemd/system/ddns-ssh.service
  sudo systemctl daemon-reload
  sudo systemctl enable --now ddns-ssh.service
  systemctl status ddns-ssh.service
  sudo tail -f /var/log/nstlddns.log
```
